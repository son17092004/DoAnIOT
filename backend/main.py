from fastapi import FastAPI, UploadFile, File, HTTPException, Form
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import JSONResponse
import uvicorn
import numpy as np
import cv2
import io
import os
import threading
from datetime import datetime
from pathlib import Path
from database import test_connection, create_student, get_all_students, create_session, mark_attendance, sessions_collection
from face_utils import detect_face, get_face_embedding, align_face, find_match, get_face_landmarks, load_antispoof_model, check_liveness

app = FastAPI(title="ESP32 Face Attendance API")

# Mount static files for frontend
os.makedirs("static", exist_ok=True)
app.mount("/static", StaticFiles(directory="static"), name="static")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Global variable to store current active session ID
CURRENT_SESSION_ID = None

# Global variable for Anti-Spoof Model
ANTISPOOF_MODEL = None

# Sử dụng relative path thay vì hard-coded Windows path
BASE_DIR = Path(__file__).resolve().parent.parent
MODEL_PATH = BASE_DIR / "training_anti_spoof" / "antispoof_model.pth"

# Trigger State Management với thread-safe lock
TRIGGER_LOCK = threading.Lock()
TRIGGER_PENDING = False
LAST_RECOGNITION_RESULT = {"timestamp": 0, "message": "Ready"}

@app.on_event("startup")
def startup_db_client():
    if test_connection():
        print("Database connected successfully")
    else:
        print("Failed to connect to database")
    
    # Load Anti-Spoof Model
    global ANTISPOOF_MODEL
    
    if MODEL_PATH.exists():
        ANTISPOOF_MODEL = load_antispoof_model(str(MODEL_PATH))
        if ANTISPOOF_MODEL:
            print("Anti-Spoofing Model Loaded!")
        else:
            print("WARNING: Could not load Anti-Spoofing Model. Liveness check will be disabled.")
    else:
        print(f"WARNING: Model file not found at {MODEL_PATH}. Liveness check disabled.")

@app.get("/")
def read_root():
    return JSONResponse(content={
        "message": "Face Attendance API Running", 
        "docs": "/docs",
        "ui": "/static/index.html"
    })

# --- Trigger Management (KHÔNG CẦN THIẾT CHO ESP-NOW, GIỮ LẠI CHO BACKWARD COMPATIBILITY) ---
@app.post("/api/trigger")
def set_trigger():
    """ 
    [DEPRECATED] Called by Main ESP (Radar) - Không còn cần thiết với ESP-NOW
    Giữ lại để backward compatibility với HTTP polling mode
    """
    global TRIGGER_PENDING
    with TRIGGER_LOCK:  # Thread-safe
        TRIGGER_PENDING = True
    print("TRIGGER: Received from Radar (HTTP mode)")
    return {"status": "ok", "message": "Trigger set"}

@app.get("/api/trigger/check")
def check_trigger():
    """ 
    [DEPRECATED] Called by Camera ESP - Không còn cần thiết với ESP-NOW
    Giữ lại để backward compatibility với HTTP polling mode
    """
    global TRIGGER_PENDING
    with TRIGGER_LOCK:  # Thread-safe
        if TRIGGER_PENDING:
            TRIGGER_PENDING = False # Consume
            print("TRIGGER: Consumed by Camera (HTTP mode)")
            return {"trigger": True}
    return {"trigger": False}

@app.get("/api/result/latest")
def get_latest_result():
    """ Called by Main ESP (Display) - Trả về kết quả nhận diện mới nhất """
    return LAST_RECOGNITION_RESULT


# --- Session Management ---
@app.post("/api/session/start")
def start_session(session_name: str = Form(...)):
    global CURRENT_SESSION_ID
    CURRENT_SESSION_ID = create_session(session_name)
    return {"status": "success", "session_id": CURRENT_SESSION_ID, "message": f"Session '{session_name}' started"}

@app.post("/api/session/stop")
def stop_session():
    global CURRENT_SESSION_ID
    CURRENT_SESSION_ID = None
    return {"status": "success", "message": "Session stopped"}

@app.get("/api/session/current")
def get_current_session():
    global CURRENT_SESSION_ID
    if CURRENT_SESSION_ID:
        return {"active": True, "session_id": CURRENT_SESSION_ID}
    return {"active": False, "session_id": None}

@app.get("/api/session/{session_id}")
def get_session_details(session_id: str):
    from bson.objectid import ObjectId
    session = sessions_collection.find_one({"_id": ObjectId(session_id)})
    if session:
        # Convert ObjectId to str for JSON serialization
        session["_id"] = str(session["_id"])
        return session
    raise HTTPException(status_code=404, detail="Session not found")

# --- Student Registration ---
@app.post("/api/register")
async def register_student(name: str = Form(...), student_id: str = Form(...), file: UploadFile = File(...)):
    contents = await file.read()
    nparr = np.frombuffer(contents, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    
    if img is None:
        raise HTTPException(status_code=400, detail="Invalid image file")

    # Detect and Align
    landmarks = get_face_landmarks(img)
    if not landmarks:
        raise HTTPException(status_code=400, detail="No face detected in image")
    
    # Use the first face found
    aligned_img = align_face(img, landmarks[0])
    
    # Extract Embedding
    embedding = get_face_embedding(aligned_img)
    if embedding is None:
        raise HTTPException(status_code=400, detail="Could not extract features from face")
        
    # Save to DB
    # Convert numpy array to list for MongoDB storage
    embedding_list = embedding.tolist()
    new_id = create_student(name, student_id, embedding_list)
    
    return {"status": "success", "student_id": new_id, "message": f"Student {name} registered successfully"}

# --- Recognition Endpoint (for ESP32) - TĂNG TỐC ---
@app.post("/api/recognize")
async def recognize_face(file: UploadFile = File(...)):
    global CURRENT_SESSION_ID, ANTISPOOF_MODEL, LAST_RECOGNITION_RESULT
    
    # QUAN TRỌNG: Chỉ nhận diện khi có session active
    if not CURRENT_SESSION_ID:
        LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "Chua bat dau"}
        return JSONResponse(
            status_code=200,
            content={"status": "error", "message": "No active session. Please start a session first."}
        )

    # Đọc file nhanh hơn với chunk size lớn
    contents = await file.read()
    nparr = np.frombuffer(contents, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    
    if img is None:
        LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "Loi anh"}
        return JSONResponse(
            status_code=200,
            content={"status": "failed", "message": "Invalid image file"}
        )

    # 1. Detect & Extract (TĂNG TỐC)
    face_locations = detect_face(img)
    if not face_locations:
        LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "Ko phat hien"}
        return JSONResponse(
            status_code=200,
            content={"status": "failed", "message": "No face detected"}
        )
    
    # Take first face
    top, right, bottom, left = face_locations[0]
    face_loc = (top, right, bottom, left)
    
    # LIVENESS CHECK
    if ANTISPOOF_MODEL:
        is_real, score = check_liveness(img, ANTISPOOF_MODEL, face_loc)
        print(f"Liveness Check: {'REAL' if is_real else 'FAKE'} (Score: {score:.4f})")
        
        if not is_real:
            # Save spoof attempt for analysis
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"SPOOF_{timestamp}.jpg"
            save_dir = "static/spoofs"
            os.makedirs(save_dir, exist_ok=True)
            cv2.imwrite(os.path.join(save_dir, filename), img)
            

            LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "GIA MAO (FAKE)"}
            
            return JSONResponse(
                status_code=200,
                content={
                    "status": "failed", 
                    "match": False, 
                    "message": "Spoof detected (Fake Face)",
                    "liveness_score": score
                }
            )

    # If Real (or no model), proceed to recognition
    embedding = get_face_embedding(img, face_location=face_loc)
    

    
    if embedding is None:
        LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "Loi trich xuat"}
        return JSONResponse(
            status_code=200,
            content={"status": "failed", "message": "Could not extract features"}
        )
        
    # 2. Match
    all_students = get_all_students()
    match = find_match(embedding, all_students, threshold=0.5)
    
    if match:
        # Save image for display
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{match['student_id']}_{timestamp}.jpg"
        save_dir = "static/captures"
        os.makedirs(save_dir, exist_ok=True)
        save_path = os.path.join(save_dir, filename)
        
        # Save the original image
        cv2.imwrite(save_path, img)
        
        image_url = f"/static/captures/{filename}"

        # 3. Mark Attendance
        marked = mark_attendance(CURRENT_SESSION_ID, match["student_id"], match["name"], image_url)
        
        # Update Last Result for LCD
        LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": match["name"]}
        
        if marked:
            print(f"MARKED: {match['name']}")
            return JSONResponse(
                status_code=200,
                content={"status": "success", "match": True, "name": match["name"], "message": "Attendance marked"}
            )
        else:
            print(f"ALREADY MARKED: {match['name']}")
            return JSONResponse(
                status_code=200,
                content={"status": "success", "match": True, "name": match["name"], "message": "Already marked"}
            )
    
    LAST_RECOGNITION_RESULT = {"timestamp": datetime.now().timestamp(), "message": "Khong nhan ra"}
    return JSONResponse(
        status_code=200,
        content={"status": "failed", "match": False, "message": "Unknown person"}
    )

# --- Security Logs Endpoint ---
@app.get("/api/spoofs")
def get_spoof_logs():
    """
    Returns list of saved spoof attempts for UI display.
    """
    spoof_dir = "static/spoofs"
    if not os.path.exists(spoof_dir):
        return []
        
    # List files, sorted by newest first
    files = sorted(os.listdir(spoof_dir), reverse=True)
    logs = []
    
    for f in files:
        if f.endswith(".jpg") or f.endswith(".png"):
            # Filename format: SPOOF_YYYYMMDD_HHMMSS.jpg
            # Parse time for display
            try:
                # Extract timestamp part
                ts_str = f.replace("SPOOF_", "").replace(".jpg", "").replace(".png", "")
                dt = datetime.strptime(ts_str, "%Y%m%d_%H%M%S")
                time_display = dt.strftime("%H:%M:%S %d/%m")
            except:
                time_display = "Unknown"
                
            logs.append({
                "filename": f,
                "url": f"/static/spoofs/{f}",
                "timestamp": time_display
            })
    
    return logs

if __name__ == "__main__":
    print("=" * 60)
    print("ESP32 Face Attendance System - Backend Server")
    print("=" * 60)
    print(f"Model path: {MODEL_PATH}")
    print(f"Base directory: {BASE_DIR}")
    print("Starting server on http://0.0.0.0:8080")
    print("API Docs: http://localhost:8080/docs")
    print("=" * 60)
    
    # Tối ưu server cho tốc độ cao
    import uvicorn
    uvicorn.run(
        "main:app", 
        host="0.0.0.0", 
        port=8080, 
        reload=True,
        timeout_keep_alive=20,   # Giảm từ 30s → 20s (đủ cho ESP32)
        limit_concurrency=20,    # Tăng từ 10 → 20 (xử lý nhiều requests hơn)
        limit_max_requests=2000, # Tăng từ 1000 → 2000
        workers=1,               # Single worker cho development
        backlog=50               # Tăng queue size
    )
