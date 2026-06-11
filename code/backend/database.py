"""
Database Layer cho Face Attendance System
- Students: Lưu thông tin sinh viên và face embeddings (nhiều ảnh)
- Classes: Quản lý lớp học
- ClassMembers: Danh sách sinh viên trong lớp
- Sessions: Phiên điểm danh theo lớp
- Attendances: Lịch sử điểm danh
"""
import os
from dotenv import load_dotenv
from pymongo import MongoClient, ASCENDING, DESCENDING
from datetime import datetime, timedelta
from bson.objectid import ObjectId
from typing import List, Dict, Optional
import pytz

load_dotenv()

# Timezone Việt Nam (UTC+7)
VN_TZ = pytz.timezone('Asia/Ho_Chi_Minh')

def get_vn_time():
    """Lấy thời gian hiện tại theo timezone Việt Nam (UTC+7)"""
    return datetime.now(VN_TZ)

MONGO_URI = os.getenv("MONGO_URI", "mongodb://localhost:27017")
DB_NAME = os.getenv("DB_NAME", "face_attendance_db")

client = MongoClient(MONGO_URI)
db = client[DB_NAME]

# Collections
students_collection = db["students"]
classes_collection = db["classes"]
class_members_collection = db["class_members"]
sessions_collection = db["sessions"]
attendances_collection = db["attendances"]
spoof_attempts_collection = db["spoof_attempts"]  # Lưu các lần giả mạo

# Create indexes for better performance
def create_indexes():
    """Tạo indexes cho các collection"""
    students_collection.create_index([("student_id", ASCENDING)], unique=True)
    classes_collection.create_index([("class_code", ASCENDING)], unique=True)
    class_members_collection.create_index([("class_id", ASCENDING), ("student_id", ASCENDING)], unique=True)
    sessions_collection.create_index([("class_id", ASCENDING), ("created_at", DESCENDING)])
    attendances_collection.create_index([("session_id", ASCENDING), ("student_id", ASCENDING)], unique=True)
    print("Database indexes created successfully")

def test_connection():
    """Test kết nối MongoDB"""
    try:
        client.admin.command('ping')
        print("✓ Connected to MongoDB!")
        create_indexes()
        return True
    except Exception as e:
        print(f"✗ MongoDB connection failed: {e}")
        return False


# ===================================
# STUDENT MANAGEMENT
# ===================================

def create_student(name: str, student_id: str, email: str = None, phone: str = None) -> str:
    """
    Tạo sinh viên mới (KHÔNG có embedding - sẽ thêm riêng)
    """
    student = {
        "name": name,
        "student_id": student_id,
        "email": email,
        "phone": phone,
        "embeddings": [],  # Danh sách embeddings từ nhiều ảnh
        "created_at": get_vn_time().replace(tzinfo=None),  # Remove timezone for MongoDB
        "updated_at": get_vn_time().replace(tzinfo=None)
    }
    result = students_collection.insert_one(student)
    return str(result.inserted_id)


def add_student_embedding(student_id: str, embedding: List[float]) -> bool:
    """
    Thêm 1 embedding mới cho sinh viên (từ 1 ảnh)
    Recommend: 3-5 ảnh với các góc độ khác nhau
    """
    result = students_collection.update_one(
        {"student_id": student_id},
        {
            "$push": {"embeddings": embedding},
            "$set": {"updated_at": get_vn_time().replace(tzinfo=None)}
        }
    )
    return result.modified_count > 0


def get_student_by_id(student_id: str) -> Optional[Dict]:
    """Lấy thông tin sinh viên theo student_id"""
    student = students_collection.find_one({"student_id": student_id})
    if student:
        student["_id"] = str(student["_id"])
    return student


def get_all_students() -> List[Dict]:
    """Lấy danh sách tất cả sinh viên"""
    students = list(students_collection.find({}))
    for s in students:
        s["_id"] = str(s["_id"])
        s["num_embeddings"] = len(s.get("embeddings", []))
    return students


def delete_student(student_id: str) -> bool:
    """Xóa sinh viên"""
    result = students_collection.delete_one({"student_id": student_id})
    return result.deleted_count > 0


def get_all_embeddings() -> List[Dict]:
    """
    Lấy tất cả embeddings của tất cả sinh viên
    Return: [{"student_id": "...", "name": "...", "embedding": [...]}, ...]
    """
    students = list(students_collection.find({}, {"student_id": 1, "name": 1, "embeddings": 1}))
    result = []
    for student in students:
        for emb in student.get("embeddings", []):
            result.append({
                "student_id": student["student_id"],
                "name": student["name"],
                "embedding": emb
            })
    return result


# ===================================
# CLASS MANAGEMENT
# ===================================

def create_class(class_name: str, class_code: str, teacher: str = None, description: str = None) -> str:
    """Tạo lớp học mới"""
    class_obj = {
        "class_name": class_name,
        "class_code": class_code,
        "teacher": teacher,
        "description": description,
        "created_at": get_vn_time().replace(tzinfo=None),
        "updated_at": get_vn_time().replace(tzinfo=None)
    }
    result = classes_collection.insert_one(class_obj)
    return str(result.inserted_id)


def get_all_classes() -> List[Dict]:
    """Lấy danh sách tất cả lớp học"""
    classes = list(classes_collection.find({}))
    for c in classes:
        c["_id"] = str(c["_id"])
        # Đếm số sinh viên trong lớp
        c["student_count"] = class_members_collection.count_documents({"class_id": c["_id"]})
    return classes


def get_class_by_id(class_id: str) -> Optional[Dict]:
    """Lấy thông tin lớp học"""
    class_obj = classes_collection.find_one({"_id": ObjectId(class_id)})
    if class_obj:
        class_obj["_id"] = str(class_obj["_id"])
        class_obj["student_count"] = class_members_collection.count_documents({"class_id": class_id})
    return class_obj


def delete_class(class_id: str) -> bool:
    """Xóa lớp học (và tất cả members)"""
    result = classes_collection.delete_one({"_id": ObjectId(class_id)})
    if result.deleted_count > 0:
        class_members_collection.delete_many({"class_id": class_id})
        return True
    return False


# ===================================
# CLASS MEMBERS MANAGEMENT
# ===================================

def add_student_to_class(class_id: str, student_id: str) -> bool:
    """Thêm sinh viên vào lớp"""
    try:
        member = {
            "class_id": class_id,
            "student_id": student_id,
            "added_at": get_vn_time().replace(tzinfo=None)
        }
        class_members_collection.insert_one(member)
        return True
    except Exception as e:
        # Duplicate key error (đã có trong lớp)
        print(f"Error adding student to class: {e}")
        return False


def remove_student_from_class(class_id: str, student_id: str) -> bool:
    """Xóa sinh viên khỏi lớp"""
    result = class_members_collection.delete_one({
        "class_id": class_id,
        "student_id": student_id
    })
    return result.deleted_count > 0


def get_class_students(class_id: str) -> List[Dict]:
    """Lấy danh sách sinh viên trong lớp"""
    members = list(class_members_collection.find({"class_id": class_id}))
    students = []
    for member in members:
        student = students_collection.find_one({"student_id": member["student_id"]})
        if student:
            student["_id"] = str(student["_id"])
            student["added_at"] = member["added_at"]
            students.append(student)
    return students


def get_student_classes(student_id: str) -> List[Dict]:
    """Lấy danh sách lớp của sinh viên"""
    members = list(class_members_collection.find({"student_id": student_id}))
    classes = []
    for member in members:
        class_obj = classes_collection.find_one({"_id": ObjectId(member["class_id"])})
        if class_obj:
            class_obj["_id"] = str(class_obj["_id"])
            classes.append(class_obj)
    return classes


# ===================================
# SESSION MANAGEMENT
# ===================================

def create_session(class_id: str, session_name: str, duration_minutes: int = 15) -> str:
    """
    Tạo phiên điểm danh cho lớp
    - duration_minutes: Thời gian cho phép sửa điểm danh (mặc định 15 phút)
    - lock_time: Thời điểm hết hạn sửa (created_at + duration)
    """
    created_at = get_vn_time().replace(tzinfo=None)
    lock_time = created_at + timedelta(minutes=duration_minutes)
    
    session = {
        "class_id": class_id,
        "session_name": session_name,
        "created_at": created_at,
        "lock_time": lock_time,  # Thời điểm không cho sửa điểm danh nữa
        "duration_minutes": duration_minutes,
        "status": "active",  # active / ended / locked
        "ended_at": None
    }
    result = sessions_collection.insert_one(session)
    return str(result.inserted_id)


def end_session(session_id: str) -> bool:
    """Kết thúc phiên điểm danh"""
    result = sessions_collection.update_one(
        {"_id": ObjectId(session_id)},
        {"$set": {"status": "ended", "ended_at": get_vn_time().replace(tzinfo=None)}}
    )
    return result.modified_count > 0


def get_session_by_id(session_id: str) -> Optional[Dict]:
    """Lấy thông tin session"""
    session = sessions_collection.find_one({"_id": ObjectId(session_id)})
    if session:
        session["_id"] = str(session["_id"])
        # Đếm số sinh viên đã điểm danh
        session["attendance_count"] = attendances_collection.count_documents({"session_id": session_id})
    return session


def get_class_sessions(class_id: str) -> List[Dict]:
    """Lấy danh sách sessions của lớp"""
    sessions = list(sessions_collection.find({"class_id": class_id}).sort("created_at", DESCENDING))
    for s in sessions:
        s["_id"] = str(s["_id"])
        s["attendance_count"] = attendances_collection.count_documents({"session_id": str(s["_id"])})
    return sessions


# ===================================
# ATTENDANCE MANAGEMENT
# ===================================

def mark_attendance(session_id: str, student_id: str, image_url: str = None, status: str = "present") -> bool:
    """
    Điểm danh sinh viên trong session
    status: "present" (có mặt), "absent" (vắng), "edited" (đã sửa)
    Return True nếu thành công, False nếu đã điểm danh rồi
    """
    try:
        attendance = {
            "session_id": session_id,
            "student_id": student_id,
            "image_url": image_url,
            "status": status,
            "timestamp": get_vn_time().replace(tzinfo=None),
            "edited_at": None,
            "edited_by": None
        }
        attendances_collection.insert_one(attendance)
        return True
    except Exception as e:
        # Duplicate key error (đã điểm danh rồi)
        print(f"Duplicate attendance: {e}")
        return False


def get_session_attendances(session_id: str) -> List[Dict]:
    """Lấy danh sách sinh viên đã điểm danh trong session"""
    attendances = list(attendances_collection.find({"session_id": session_id}).sort("timestamp", ASCENDING))
    result = []
    for att in attendances:
        student = students_collection.find_one({"student_id": att["student_id"]})
        if student:
            result.append({
                "student_id": att["student_id"],
                "name": student["name"],
                "timestamp": att["timestamp"],
                "image_url": att.get("image_url")
            })
    return result


def check_attendance_status(session_id: str, student_id: str) -> bool:
    """Kiểm tra sinh viên đã điểm danh chưa"""
    return attendances_collection.count_documents({
        "session_id": session_id,
        "student_id": student_id
    }) > 0


def update_attendance_status(session_id: str, student_id: str, new_status: str, edited_by: str = "teacher") -> bool:
    """
    Cập nhật trạng thái điểm danh (chỉ trong thời gian lock_time)
    new_status: "present" hoặc "absent"
    """
    # Kiểm tra session còn cho phép sửa không
    session = sessions_collection.find_one({"_id": ObjectId(session_id)})
    if not session:
        return False
    
    now = get_vn_time().replace(tzinfo=None)
    if now > session["lock_time"]:
        print(f"Session locked! Cannot edit after {session['lock_time']}")
        return False
    
    # Nếu chưa có attendance record → Tạo mới
    existing = attendances_collection.find_one({
        "session_id": session_id,
        "student_id": student_id
    })
    
    if not existing:
        # Tạo mới (trường hợp giảng viên điểm danh thủ công)
        return mark_attendance(session_id, student_id, status=new_status)
    else:
        # Update existing
        result = attendances_collection.update_one(
            {"session_id": session_id, "student_id": student_id},
            {
                "$set": {
                    "status": new_status,
                    "edited_at": now,
                    "edited_by": edited_by
                }
            }
        )
        return result.modified_count > 0


def is_session_locked(session_id: str) -> bool:
    """Kiểm tra session có bị khóa (hết thời gian sửa) không"""
    session = sessions_collection.find_one({"_id": ObjectId(session_id)})
    if not session:
        return True
    return get_vn_time().replace(tzinfo=None) > session["lock_time"]


def get_attendance_report(class_id: str, session_id: str) -> Dict:
    """
    Tạo báo cáo điểm danh: danh sách có mặt và vắng mặt
    """
    # Lấy tất cả sinh viên trong lớp
    class_students = get_class_students(class_id)
    
    # Lấy danh sách đã điểm danh (bất kể status)
    all_attendances = list(attendances_collection.find({"session_id": session_id}))
    
    # Phân loại theo status
    present = []
    absent_marked = []  # Vắng có record (giảng viên đánh dấu)
    
    attended_map = {}
    for att in all_attendances:
        attended_map[att["student_id"]] = att
    
    # Xử lý từng sinh viên
    for student in class_students:
        sid = student["student_id"]
        student_info = {
            "student_id": sid,
            "name": student["name"]
        }
        
        if sid in attended_map:
            att = attended_map[sid]
            student_info["timestamp"] = att["timestamp"]
            student_info["image_url"] = att.get("image_url")
            student_info["edited_at"] = att.get("edited_at")
            
            if att["status"] == "present":
                present.append(student_info)
            else:  # absent
                absent_marked.append(student_info)
        else:
            # Không có record → Vắng (chưa điểm danh)
            absent_marked.append(student_info)
    
    return {
        "total_students": len(class_students),
        "present_count": len(present),
        "absent_count": len(absent_marked),
        "present": present,
        "absent": absent_marked
    }


# ===================================
# SPOOF ATTEMPTS MANAGEMENT
# ===================================

def log_spoof_attempt(session_id: str, image_url: str, confidence: float = 0.0, student_match: str = None) -> bool:
    """
    Lưu lại lần giả mạo
    - session_id: Session hiện tại
    - image_url: Đường dẫn ảnh giả mạo
    - confidence: Độ tin cậy từ anti-spoof model
    - student_match: Nếu có match với sinh viên nào đó
    """
    try:
        spoof = {
            "session_id": session_id,
            "image_url": image_url,
            "confidence": confidence,
            "student_match": student_match,
            "timestamp": get_vn_time().replace(tzinfo=None),
            "reviewed": False  # Giảng viên đã xem chưa
        }
        spoof_attempts_collection.insert_one(spoof)
        return True
    except Exception as e:
        print(f"Error logging spoof attempt: {e}")
        return False


def get_session_spoof_attempts(session_id: str) -> List[Dict]:
    """Lấy danh sách các lần giả mạo trong session"""
    spoofs = list(spoof_attempts_collection.find({"session_id": session_id}).sort("timestamp", DESCENDING))
    for s in spoofs:
        s["_id"] = str(s["_id"])
    return spoofs


def mark_spoof_reviewed(spoof_id: str) -> bool:
    """Đánh dấu giảng viên đã xem spoof attempt"""
    result = spoof_attempts_collection.update_one(
        {"_id": ObjectId(spoof_id)},
        {"$set": {"reviewed": True}}
    )
    return result.modified_count > 0
