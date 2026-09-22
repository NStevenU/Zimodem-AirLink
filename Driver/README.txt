==============================================================================
 AirLink 56K Modem Driver for Windows 95 / 98 / ME / 2000 / XP (32-bit)
 Nexisson Tech Co., Ltd.
 Reference Architecture: U.S. Robotics 56K Faxmodem External (USR5686E)
==============================================================================

[지원 운영체제 (Supported OS)]
- Windows 95 / 95 OSR2
- Windows 98 / 98 SE
- Windows Millennium Edition (ME)
- Windows NT 4.0 (SP6)
- Windows 2000 Professional / Server
- Windows XP Home / Professional (32-bit)
※ Windows Vista 이상의 최신 OS(비스타, 7, 8, 10, 11 및 64비트)는 지원 대상에서 제외됩니다.


[드라이버 파일 설명 (Driver Files)]
1. AIRLINK.INF (통합 드라이버 - 권장) :
   - 설치 시 제어판 목록에서 아래 2개 모델 중 하나를 바로 선택 가능:
     • "AirLink 56K (RTS/CTS)" : 하드웨어 흐름제어 (RTS/CTS)
     • "AirLink 56K (XON/XOFF)" : 소프트웨어 흐름제어 (XON/XOFF)

2. AIRLINK_HW.INF (하드웨어 흐름제어 전용) :
   - 모델명: "AirLink 56K (RTS/CTS)"
   - 흐름 제어: 하드웨어 RTS/CTS (AT&K3) 기본 적용
   - 추천 환경: MAX3232에 RTS/CTS 핀 배선이 연결된 풀 시리얼 케이블 환경

3. AIRLINK_SW.INF (소프트웨어 흐름제어 전용) :
   - 모델명: "AirLink 56K (XON/XOFF)"
   - 흐름 제어: 소프트웨어 XON/XOFF (AT&K4) 기본 적용
   - 추천 환경: 기본 4핀 MAX3232 모듈 (TX, RX, GND만 연결된 3선식 시리얼 케이블 환경)


[아키텍처 및 정석 정책 안내]
1. 통신 속도 정책 (USR5686E 공식 정석 준수):
   - 비표준 강제 하드코딩(DCB, MaximumPortSpeed)을 배제하고, Unimodem Properties의
     최대 DTE 속도(dwMaxDTERate = 115200 bps) 상한선 규격을 적용했습니다.
   - 기본 설치 시에는 최고 속도인 115,200 bps로 자동 설정되며,
     구형 PC(FIFO 버퍼가 없는 16450/8250 UART 장착 386/486 PC) 사용 시에는
     제어판 모뎀 등록정보에서 사용자가 속도를 57600, 38400, 19200 등으로
     자유롭게 낮춰 설정할 수 있습니다.

2. Zimodem 펌웨어 1:1 완벽 연동:
   - Zimodem 펌웨어 소스 전수 조사를 바탕으로 지원 명령어만 배타적으로 구성:
     • 초기화: ATZ (리셋), ATE0Q0V1 (에코 끔, 결과코드 켬, 상세 응답)
     • 흐름제어: AT&K3 (RTS/CTS), AT&K4 (XON/XOFF), AT&K0 (끔)
     • 스피커: ATM0 (음소거), ATM1 (다이얼 중 켬), ATM2 (항상 켬)
     • 발신: ATDT (톤 다이얼, 사운드 출력 연동), 01481 (윈도우 95 다이얼업 네트워킹 PPP 모드)
   - Zimodem에서 에러를 내는 타사 전용 명령어(&C1, &D2, &H1, \N3, %C1 등)는 일체 전송되지 않습니다.


[Windows 95 / 98 / ME / 2000 / XP 설치 및 재설치 방법]

※ 중요: 이전에 설치된 AirLink 56K 모뎀이 제어판에 등록되어 있다면,
  반드시 먼저 [제거]한 후 다시 설치해야 새 INF 설정이 반영됩니다.

1. 기존 모뎀 제거 (재설치 시 필수)
   - [시작] -> [설정] -> [제어판] -> [모뎀] 실행
   - 목록에서 기존 "AirLink 56K" 선택 후 [삭제] 클릭

2. 드라이버 파일 복사
   - Driver 폴더의 파일(AIRLINK.INF 등)을 플로피 디스크, 가상머신 공유 폴더,
     ISO 이미지 등을 통해 PC로 복사합니다.

3. 새 모뎀 수동 추가
   - [제어판] -> [모뎀] 실행 -> [추가] 버튼 클릭
   - "모뎀을 검색하지 않고 목록에서 직접 선택" 체크박스 체크 후 [다음] 클릭
   - [디스크 있음...] 버튼 클릭 -> INF 파일이 있는 폴더 지정 후 [확인] 클릭

4. 모델 및 포트 지정
   - 제조사: "Nexisson Tech Co., Ltd."
   - 모델: 배선 방식에 따라 "AirLink 56K (RTS/CTS)" 또는 "AirLink 56K (XON/XOFF)" 선택
   - AirLink가 연결된 통신 포트(예: 통신 포트 COM1) 선택 후 [다음] -> [마침]

5. 정상 동작 확인
   - [제어판] -> [모뎀] -> 모뎀 선택 후 [등록 정보] 클릭
     -> 정상적으로 포트, 최대 속도(115200), [연결] 탭 대화상자가 열리는지 확인
   - 새롬 데이타맨 98/Pro 등 통신 프로그램에서 모뎀 목록에 "AirLink 56K"가 정상 표시되는지 확인

==============================================================================
 (C) 2026 Nexisson Tech Co., Ltd. All rights reserved.
==============================================================================
