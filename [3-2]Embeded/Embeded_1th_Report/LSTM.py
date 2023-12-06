import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
from sklearn.preprocessing import StandardScaler
import tensorflow as tf

data = pd.read_excel('./embeded.xls')
start_time = datetime.strptime('1/17/2013 2:41:00 PM', '%m/%d/%Y %I:%M:%S %p')
data.rename(columns={'온도': 'Temperature', '습도': 'Humidity'}, inplace=True)
data = data.sort_values(['Time', 'No.'], ascending=[True, True])
data.drop(['No.', 'Time'], axis=1, inplace=True)
data = data.replace({',': '.'}, regex=True)
data = data.replace({'RPM': -1}, regex=True)

# zero값 삭제
zero_value_indices = data[(data['Temperature'] == 0) | (data['Humidity'] == 0)].index
data.drop(zero_value_indices, inplace=True)
data.drop(data.columns.difference(['Temperature', 'Humidity']), axis=1, inplace=True)

# 데이터 인덱스를 10초 간격으로 생성
data.index = pd.date_range(start=start_time, periods=len(data), freq='10S')

def handle_outliers_iqr(data, column_name, k=1.5):
    Q1 = data[column_name].quantile(0.25)
    Q3 = data[column_name].quantile(0.75)
    IQR = Q3 - Q1
    lower_bound = Q1 - k * IQR
    upper_bound = Q3 + k * IQR
    data[column_name] = np.where((data[column_name] < lower_bound) | (data[column_name] > upper_bound), np.nan, data[column_name])
    data[column_name].fillna(method='ffill', inplace=True)
    return data

columns_to_process = ['Temperature', 'Humidity']
for column in columns_to_process:
    data = handle_outliers_iqr(data, column)

data.dropna(inplace=True)
print(data)

# 데이터 프레임 >> NumPy 배열
data_array = data[['Temperature', 'Humidity']].values

# 데이터 >> 훈련 & 테스트 데이터
train_size = int(len(data_array) * 0.8)
train_data, test_data = data_array[:train_size], data_array[train_size:]

# 데이터 스케일링
scaler = StandardScaler()
train_data = scaler.fit_transform(train_data)
test_data = scaler.transform(test_data)

def create_sequences(data, seq_length):
    sequences = []
    for i in range(len(data) - seq_length):
        sequence = data[i:i+seq_length]
        sequences.append(sequence)
    return np.array(sequences)

seq_length = 10
train_sequences = create_sequences(train_data, seq_length)
test_sequences = create_sequences(test_data, seq_length)

X_train, y_train = train_sequences[:, :-1], train_sequences[:, -1]
X_test, y_test = test_sequences[:, :-1], test_sequences[:, -1]

hidden_units = 64
input_shape = (seq_length - 1, 2)

input_layer = tf.keras.layers.Input(shape=input_shape)
lstm_layer = tf.keras.layers.LSTM(hidden_units)(input_layer)
output_layer = tf.keras.layers.Dense(2)(lstm_layer)

model = tf.keras.models.Model(inputs=input_layer, outputs=output_layer)
model.compile(optimizer='adam', loss='mean_squared_error')

# 모델 훈련
model.fit(X_train, y_train, epochs=100, batch_size=32)

# 데이터 예측
predictions = model.predict(X_test)

# 역 스케일링
predictions = scaler.inverse_transform(predictions)
train_data = scaler.inverse_transform(train_data)
test_data = scaler.inverse_transform(test_data)

# matplotlib 시간 설정
def get_time_indices(start_time, train_len, test_len):
    train_time_indices = [start_time + timedelta(seconds=i * 10) for i in range(train_len)]
    test_time_indices = [start_time + timedelta(seconds=(train_len + i) * 10) for i in range(test_len)]
    return train_time_indices, test_time_indices

train_time_indices, test_time_indices = get_time_indices(start_time, len(train_data), len(test_data))

# 그래프 출력
fig, axs = plt.subplots(2, 1, figsize=(12, 8))
# 온도 그래프
axs[0].plot(train_time_indices, train_data[:, 0], label='Train (Temperature)', color='blue')
axs[0].plot(test_time_indices, test_data[:, 0], label='Test (Temperature)', color='green')
axs[0].plot(test_time_indices[seq_length:], predictions[:, 0], label='Predicted (Temperature)', color='cyan')
axs[0].set_title('Temperature Prediction')
axs[0].set_xlabel('Time')
axs[0].set_ylabel('Temperature')
axs[0].legend()

# 습도 그래프
axs[1].plot(train_time_indices, train_data[:, 1], label='Train (Humidity)', color='orange')
axs[1].plot(test_time_indices, test_data[:, 1], label='Test (Humidity)', color='red')
axs[1].plot(test_time_indices[seq_length:], predictions[:, 1], label='Predicted (Humidity)', color='magenta')
axs[1].set_title('Humidity Prediction')
axs[1].set_xlabel('Time')
axs[1].set_ylabel('Humidity')
axs[1].legend()

plt.tight_layout()
plt.xticks(rotation=45)  # x축 레이블 회전
plt.show()


# 측정된 데이터의 마지막 시간, 온도, 습도 출력
last_train_time = train_time_indices[-1]
last_train_temperature = train_data[-1, 0]
last_train_humidity = train_data[-1, 1]

print("현재 시간:", last_train_time)
print("온도:", last_train_temperature)
print("습도:", last_train_humidity)

# 예측 데이터의 마지막 시간, 온도, 습도 출력
last_pred_time = test_time_indices[-1]
last_pred_temperature = predictions[-1, 0]
last_pred_humidity = predictions[-1, 1]

print("\n1시간 후 - Last Time:", last_pred_time)
print("온도:", last_pred_temperature)
print("습도:", last_pred_humidity)


# 예측값을 데이터프레임으로 변환
predictions_df = pd.DataFrame({'Time': test_time_indices[seq_length:], 'Temperature': predictions[:, 0], 'Humidity': predictions[:, 1]})

# CSV 파일로 저장
predictions_df.to_csv('predictions.csv', index=False)


import firebase_admin
from firebase_admin import credentials
from firebase_admin import db

# Firebase 프로젝트 서비스 계정 키 파일 경로
cred = credentials.Certificate('./mykey.json')
firebase_admin.initialize_app(cred, {
    'databaseURL': "https://embaded-737eb-default-rtdb.firebaseio.com"
})

# 예측값을 데이터프레임으로 변환
predictions_df = pd.DataFrame({'Time': test_time_indices[seq_length:], 'Temperature': predictions[:, 0], 'Humidity': predictions[:, 1]})

# Timestamp를 문자열로 변환
predictions_df['Time'] = predictions_df['Time'].apply(lambda x: x.strftime('%Y-%m-%d %H:%M:%S'))

# 습도가 0.495 이상인 경우의 시간 저장
high_humidity_times = predictions_df[predictions_df['Humidity'] >= 0.495]['Time'].tolist()

# Firebase Realtime Database에 데이터 업로드
ref = db.reference('/predictions')  # 업로드할 위치 지정
ref.set(predictions_df.to_dict())  # 데이터프레임을 딕셔너리로 변환하여 업로드

# 습도가 0.495 이상인 경우의 시간만 따로 Firebase에 저장
warning_ref = db.reference('/warning')
for time in high_humidity_times:
    warning_ref.push().set({'time': time})