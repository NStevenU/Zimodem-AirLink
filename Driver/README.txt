==============================================================================
 AirLink 56K Modem Driver for Windows 95 / 98 / Me / 2000 / XP
 Nexisson Tech Co., Ltd.
==============================================================================

[드라이버 파일 설명 (Driver Files)]
1. AIRLINK_HW.INF :
   - 모델명: "AirLink 56K (Hardware)"
   - 흐름 제어: 하드웨어 CTS/RTS (AT&K3) 기본 적용
   - 추천 환경: 5선 이상 연결된 풀 시리얼 케이블 (RTS/CTS 핀 사용 가능 시)

2. AIRLINK_SW.INF :
   - 모델명: "AirLink 56K (Software)"
   - 흐름 제어: 소프트웨어 XON/XOFF (AT&K4) 기본 적용
   - 추천 환경: 3선식 시리얼 케이블 (TX, RX, GND만 연결된 경우)


[Windows 95 / 98 설치 방법 (Installation Guide)]

1. Windows 95/98 환경 준비
   - 이 Driver 폴더의 파일(AIRLINK_HW.INF 또는 AIRLINK_SW.INF)을
     플로피 디스크, CD-ROM 이미지(ISO), 가상머신 공유 폴더 등으로 
     Windows 95/98 PC로 복사합니다.

2. 제어판 모뎀 설정
   - [시작] -> [설정] -> [제어판] 실행
   - [모뎀] 아이콘 더블 클릭
   - 기존에 잡혀있던 표준 모뎀이 있다면 선택 후 [제거]
   - [추가(Add...)] 버튼 클릭

3. 드라이버 수동 지정
   - "모뎀을 검색하지 않고 목록에서 직접 선택(Don't detect my modem; I will select it from a list)"
     항목을 체크한 후 [다음] 클릭
   - [디스크 있음...(Have Disk...)] 버튼 클릭
   - INF 파일이 위치한 경로(예: A:\, C:\DRIVER 등)를 지정하고 [확인] 클릭

4. 모델 및 포트 선택
   - 제조사: "Nexisson Tech Co., Ltd."
   - 모델: "AirLink 56K (Hardware)" 또는 "AirLink 56K (Software)" 선택 후 [다음]
   - AirLink가 연결된 통신 포트(예: 통신 포트 COM1) 선택 후 [다음]
   - [마침]을 누르면 설치가 완료됩니다.

5. 포트 속도 확인
   - 제어판 -> 모뎀 -> "AirLink 56K" 선택 -> [등록 정보(Properties)]
   - '최대 속도(Maximum Speed)'가 115200으로 설정되어 있는지 확인합니다.
   - [진단(Diagnostics)] 탭에서 해당 COM 포트 선택 후 [추가 정보(More Info)]를 누르면
     ATI 정보 및 버전 정보가 성공적으로 조회됩니다.

==============================================================================
 (C) 2026 Nexisson Tech Co., Ltd. All rights reserved.
==============================================================================
