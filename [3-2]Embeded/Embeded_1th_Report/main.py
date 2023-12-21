import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
#import Adafruit_DHT
import time
import random

# Firebase 프로젝트의 서비스 계정 키 경로
cred = credentials.Certificate('./mykey.json')

# Firebase 앱 초기화
firebase_admin.initialize_app(cred, {
    'databaseURL': "https://embaded-737eb-default-rtdb.firebaseio.com"
})

# 데이터베이스 reference 가져오기
ref = db.reference('/message')

while True:
    # 무작위 온도와 습도 생성
    temperature = round(random.uniform(20, 22), 2)
    humidity = round(random.uniform(40, 50), 2)

    time_hhmmss = time.strftime('%H:%M:%S')
    date_mmddyyyy = time.strftime('%d/%m/%Y')
    data = {
        "Date": date_mmddyyyy,
        "Time": time_hhmmss,
        "Temperature": temperature,
        "Humidity": humidity
    }

    # Firebase 데이터베이스에 데이터 푸시
    ref.push(data)
    print("Temp: %f -- Humidity: %f -- Date: %s  -- Time: %s" % (temperature, humidity, date_mmddyyyy, time_hhmmss))
    time.sleep(5)