import firebase_admin
from firebase_admin import credentials, db
import pandas as pd

# Firebase 프로젝트 서비스 계정 키 파일 경로
cred = credentials.Certificate('./mykey.json')

# Firebase 앱 초기화
firebase_admin.initialize_app(cred, {
    'databaseURL': "https://embaded-737eb-default-rtdb.firebaseio.com"
})

# Firebase 데이터베이스에 연결하여 데이터 가져오기
ref = db.reference('/')
data = ref.get()

# 데이터가 있는 경우 처리
if data:
    # 데이터를 리스트로 변환
    data_list = []
    for key, value in data['message'].items():
        temp = value.get('Temperature')
        humidity = value.get('Humidity')
        date = value.get('Date')
        time = value.get('Time')
        data_list.append([date, time, temp, humidity])

    # Pandas DataFrame으로 변환
    df = pd.DataFrame(data_list, columns=['Date', 'Time', 'Temperature', 'Humidity'])

    # 엑셀 파일로 저장
    df.to_excel('firebase_data.xlsx', index=False)
    print("데이터를 엑셀 파일로 저장했습니다.")
else:
    print("데이터가 없습니다.")