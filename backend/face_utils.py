import face_recognition
import cv2
import numpy as np
import math
import torch
import torch.nn as nn
from torchvision import models, transforms


def detect_face(image_np):
    """
    Detects face locations in an image.
    Returns list of bounding boxes (top, right, bottom, left).
    """
    rgb_image = cv2.cvtColor(image_np, cv2.COLOR_BGR2RGB)
    face_locations = face_recognition.face_locations(rgb_image)
    return face_locations

def get_face_landmarks(image_np, face_locations=None):
    """
    Returns face landmarks for alignment.
    """
    rgb_image = cv2.cvtColor(image_np, cv2.COLOR_BGR2RGB)
    return face_recognition.face_landmarks(rgb_image, face_locations)

def align_face(image_np, landmarks):
    """
    Aligns face based on eye coordinates.
    landmarks: dict of landmarks (from face_recognition)
    """
    if not landmarks:
        return image_np
    
    # Get eye coordinates
    left_eye = landmarks['left_eye']
    right_eye = landmarks['right_eye']
    
    # Calculate center of eyes
    left_eye_center = np.mean(left_eye, axis=0).astype("int")
    right_eye_center = np.mean(right_eye, axis=0).astype("int")
    
    # Calculate angle
    dy = right_eye_center[1] - left_eye_center[1]
    dx = right_eye_center[0] - left_eye_center[0]
    angle = np.degrees(np.arctan2(dy, dx))
    
    # Calculate center of rotation
    eye_center = (int((left_eye_center[0] + right_eye_center[0]) // 2),
                  int((left_eye_center[1] + right_eye_center[1]) // 2))
    
    # Get rotation matrix
    M = cv2.getRotationMatrix2D(eye_center, angle, 1.0)
    
    # Rotate image
    (h, w) = image_np.shape[:2]
    aligned_face = cv2.warpAffine(image_np, M, (w, h), flags=cv2.INTER_CUBIC)
    
    return aligned_face

def get_face_embedding(image_np, face_location=None):
    """
    Extracts 128D face embedding.
    If face_location is provided, extracts for that specific face.
    Otherwise, detects faces and extracts for the first one found.
    """
    rgb_image = cv2.cvtColor(image_np, cv2.COLOR_BGR2RGB)
    
    if face_location:
        known_locations = [face_location]
    else:
        known_locations = face_recognition.face_locations(rgb_image)
        if not known_locations:
            return None
            
    encodings = face_recognition.face_encodings(rgb_image, known_face_locations=known_locations)
    if encodings:
        return encodings[0]
    return None

def find_match(input_embedding, known_students, threshold=0.5):
    """
    Compares input_embedding with a list of known_students.
    known_students: list of dicts {name, student_id, embedding}
    Returns: matched_student (dict) or None
    """
    if input_embedding is None or not known_students:
        return None
        
    known_embeddings = [np.array(s['embedding']) for s in known_students]
    
    # Calculate distances
    distances = face_recognition.face_distance(known_embeddings, input_embedding)
    
    # Find best match
    min_distance_idx = np.argmin(distances)
    min_distance = distances[min_distance_idx]
    
    if min_distance < threshold:
        return known_students[min_distance_idx]
        
    return None

# --- Anti-Spoofing Logic ---

def get_antispoof_model(device):
    """
    Returns the MobileNetV2 model architecture.
    """
    model = models.mobilenet_v2(weights=None) # No need to download pretrained weights again
    model.classifier[1] = nn.Linear(model.last_channel, 2)
    model = model.to(device)
    return model

def load_antispoof_model(model_path):
    """
    Loads the trained anti-spoofing model from .pth file.
    """
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Loading Anti-Spoofing model from {model_path} to {device}...")
    
    try:
        model = get_antispoof_model(device)
        model.load_state_dict(torch.load(model_path, map_location=device))
        model.eval()
        print("Model loaded successfully.")
        return model
    except Exception as e:
        print(f"Error loading model: {e}")
        return None

def check_liveness(image_np, model, face_location=None, threshold=0.5):
    """
    Checks if a face is Real or Spoof.
    Returns:
        is_real (bool): True if Real, False if Spoof
        score (float): Probability of being Real (0.0 - 1.0)
    """
    if model is None:
        print("Warning: Anti-spoofing model is not loaded.")
        return True, 1.0 # Fail-safe: assume real if no model (or handle as error)

    try:
        # Preprocessing
        # 1. Crop face
        if face_location:
            top, right, bottom, left = face_location
            # Add some margin
            h, w = image_np.shape[:2]
            pad_h = int((bottom - top) * 0.2)
            pad_w = int((right - left) * 0.2)
            
            top = max(0, top - pad_h)
            bottom = min(h, bottom + pad_h)
            left = max(0, left - pad_w)
            right = min(w, right + pad_w)
            
            face_img = image_np[top:bottom, left:right]
        else:
            face_img = image_np

        if face_img.size == 0:
             return False, 0.0

        # 2. Resize and Normalize (Must match training transforms)
        # Train transform: Resize(224), ToTensor, Normalize
        face_img = cv2.cvtColor(face_img, cv2.COLOR_BGR2RGB)
        pil_img = transforms.ToPILImage()(face_img)
        
        transform = transforms.Compose([
            transforms.Resize((224, 224)),
            transforms.ToTensor(),
            transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
        ])
        
        input_tensor = transform(pil_img).unsqueeze(0) # Add batch dimension
        
        # Inference
        device = next(model.parameters()).device
        input_tensor = input_tensor.to(device)
        
        with torch.no_grad():
            outputs = model(input_tensor)
            probabilities = torch.nn.functional.softmax(outputs, dim=1)
            
            # Assuming Class 1 is Real, Class 0 is Spoof (Based on our training code)
            real_score = probabilities[0][1].item()
            
        is_real = real_score > threshold
        return is_real, real_score

    except Exception as e:
        print(f"Error in check_liveness: {e}")
        return True, 1.0 # Default fallback
