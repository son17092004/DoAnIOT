import requests
import sys
import os

# Configuration
API_URL = "http://localhost:8080/api"
IMAGE_PATH = "test_face.jpg" # You need to have an image here

def simulate_esp32(image_path):
    if not os.path.exists(image_path):
        print(f"Error: File {image_path} not found.")
        print("Please place a face image named 'test_face.jpg' in this folder.")
        return

    print(f"Sending {image_path} to {API_URL}/recognize...")
    
    try:
        files = {'file': open(image_path, 'rb')}
        response = requests.post(f"{API_URL}/recognize", files=files)
        
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.json()}")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        simulate_esp32(sys.argv[1])
    else:
        simulate_esp32(IMAGE_PATH)
