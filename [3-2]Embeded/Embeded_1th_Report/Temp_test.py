import numpy as np
import matplotlib.pyplot as plt

# 낮과 밤에 따른 온도 변화 시뮬레이션
def generate_temperature_variation():
    num_time_points = 24  # 1일을 24시간으로 나눔
    time = np.linspace(0, 2 * np.pi, num_time_points)  # 24시간을 2파이로 표현

    # 낮과 밤에 따른 온도 변화 설정
    day_temperature = 4 * np.sin(time - np.pi / 2) + 18  # 낮 온도: 14 ~ 22도 사이 변동
    night_temperature = -2 * np.sin(time - np.pi / 2) + 18  # 밤 온도: 14 ~ 18도 사이 변동

    # 낮과 밤 시간대 구분
    time_of_day = np.where(np.logical_and(time >= np.pi / 2, time <= 3 * np.pi / 2), day_temperature, night_temperature)
    return time_of_day

# 온도 데이터 생성
temperature_data = generate_temperature_variation()

# 그래프 출력
plt.figure(figsize=(8, 5))
plt.plot(temperature_data, label='Temperature Variation')
plt.title('Temperature Variation Throughout the Day')
plt.xlabel('Time (hours)')
plt.ylabel('Temperature (°C)')
plt.legend()
plt.grid(True)
plt.show()