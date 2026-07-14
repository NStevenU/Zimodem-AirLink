<p align="center">
  <img src="AirLink Logo.svg" width="600" alt="AirLink Serial 56K Logo">
</p>

<h1 align="center">AirLink Serial 56K - Super Micro Modem</h1>

<p align="center">
  <strong>ESP32-C3 기반 IBM PC 전용 고품질 WiFi 모뎀 에뮬레이터 (한국어 지원!)</strong><br>
  <em>Based on <a href="https://github.com/bozimmerman/Zimodem">Zimodem 4.0</a> by Bo Zimmerman</em>
</p>

---

## 📌 Overview
**AirLink Serial 56K**는 Zimodem 펌웨어를 기반으로 최적화와 수정, 개선을 거친 WiFi 모뎀 펌웨어입니다. 레트로 IBM PC 환경에 완벽하게 호환되도록 설계되었으며, **ESP32-C3** 칩셋의 성능을 활용하여 실제 모뎀의 감성적인 사운드와 현대적인 편의성(VT100 메뉴, 한글 지원 등)을 동시에 제공합니다.

## ✨ Key Modifications & Features
* **최신 개발 환경 대응**: 최신 Arduino IDE, ESP-IDF v5.0 이상 대응
* **AirLink 독자 브랜드 적용**: ESP32-C3, MAX3232 (RS232 변환), Buzzer를 결합한 전용 하드웨어 설계
* **IBM PC 전용 최적화**: 범용 레트로 PC 지원 코드를 덜어내고, ESP32-C3에 맞지 않는 불필요한 명령어 비활성화
* **모뎀 사운드 에뮬레이션 (Modem Sound Emulation)**: 
  * 전화 벨소리(Ringing), DTMF 다이얼 톤, V.90 모뎀 접속음을 추가 및 유기적 연동
  * 실제 문자열 접속 주소 입력 시, 사운드 출력을 위해 DTMF 음으로 자동 대체하여 레트로 감성 극대화
* **VT100 대화형 설정 메뉴**: 터미널에서 직관적으로 모뎀을 세팅할 수 있는 `AT+CONFIG` 메뉴 전면 개편
* **완벽한 한국어(EUC-KR) 지원**: 설정 메뉴 및 출력 메시지에서 영어/한국어 선택 기능 추가
* **사용자 편의성 강화**: 자체 도움말 기능(`AT+HELP`) 및 기타 마이너 편의 기능 탑재

## 🛠️ Hardware Requirements
* **Microcontroller**: ESP32-C3
* **Serial Communication**: MAX3232 (RS232 to TTL)
* **Audio**: Buzzer (for Sound Emulation)

## 📜 Credits & License
* **Developer**: NSteven, Republic of Korea ([GitHub](https://github.com/NStevenU/))
* **Original Firmware**: Zimodem 4.0 (C)2016-2026 Bo Zimmerman ([GitHub](https://github.com/bozimmerman/Zimodem))
* **License**: [Apache License 2.0](LICENSE)
