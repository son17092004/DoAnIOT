import os
from dotenv import load_dotenv
from pymongo import MongoClient
from datetime import datetime
from bson.objectid import ObjectId

load_dotenv()

MONGO_URI = os.getenv("MONGO_URI", "mongodb://localhost:27017")
DB_NAME = os.getenv("DB_NAME", "face_attendance_db")

client = MongoClient(MONGO_URI)
db = client[DB_NAME]

students_collection = db["students"]
sessions_collection = db["sessions"]

def test_connection():
    try:
        client.admin.command('ping')
        print("Pinged your deployment. You successfully connected to MongoDB!")
        return True
    except Exception as e:
        print(e)
        return False

def create_student(name, student_id, embedding):
    """
    Creates a new student record.
    embedding: list of floats (128D vector)
    """
    student = {
        "name": name,
        "student_id": student_id,
        "embedding": embedding,
        "created_at": datetime.utcnow()
    }
    result = students_collection.insert_one(student)
    return str(result.inserted_id)

def get_all_students():
    """
    Retrieves all students with their embeddings.
    """
    return list(students_collection.find({}, {"_id": 1, "name": 1, "student_id": 1, "embedding": 1}))

def create_session(session_name):
    """
    Creates a new attendance session.
    """
    session = {
        "session_name": session_name,
        "date": datetime.utcnow(),
        "attendees": []
    }
    result = sessions_collection.insert_one(session)
    return str(result.inserted_id)

def mark_attendance(session_id, student_id, student_name, image_url=None):
    """
    Marks a student as present in a session.
    Avoids duplicate entries for the same student in the same session.
    """
    # Check if already present
    session = sessions_collection.find_one({
        "_id": ObjectId(session_id),
        "attendees.student_id": student_id
    })
    
    if session:
        return False # Already marked

    attendee = {
        "student_id": student_id,
        "name": student_name,
        "image_url": image_url,
        "timestamp": datetime.utcnow()
    }
    
    sessions_collection.update_one(
        {"_id": ObjectId(session_id)},
        {"$push": {"attendees": attendee}}
    )
    return True
