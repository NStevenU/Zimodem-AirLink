/*
   Copyright 2016-2026 Bo Zimmerman

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

void ZConfig::switchTo() {
  debugPrintf("\r\nMode:Config\r\n");
  currMode = &configMode;
  serial.setFlowControlType(commandMode.serial.getFlowControlType());
  serial.setPetsciiMode(commandMode.serial.isPetsciiMode());
  savedEcho = commandMode.doEcho;
  newListen = commandMode.preserveListeners;
  commandMode.doEcho = true;
  serverSpec.port = 6502;
  serverSpec.flagsBitmap = commandMode.getConfigFlagBitmap() & (~FLAG_ECHO);
  if (servs)
    serverSpec = *servs;
  serial.setXON(true);
  showMenu = true;
  EOLN = commandMode.EOLN;
  EOLNC = EOLN.c_str();
  currState = ZCFGMENU_MAIN;
  lastNumber = 0;
  lastAddress = "";
  lastOptions = "";
  settingsChanged = false;
  lastNumNetworks = 0;
  lastIP = (staticIP != null) ? *staticIP : WiFi.localIP();
  lastDNS = (staticDNS != null) ? *staticDNS : IPAddress(192, 168, 0, 1);
  lastGW = (staticGW != null) ? *staticGW : IPAddress(192, 168, 0, 1);
  lastSN = (staticSN != null) ? *staticSN : IPAddress(255, 255, 255, 0);
}

void ZConfig::serialIncoming() {
  bool crReceived = commandMode.readSerialStream();
  if (crReceived) {
    doModeCommand();
  }
}

void ZConfig::switchBackToCommandMode() {
  debugPrintf("\r\nMode:Command\r\n");
  commandMode.doEcho = savedEcho;
  currMode = &commandMode;
}

void ZConfig::doModeCommand() {
  String cmd = commandMode.getNextSerialCommand();
  char c = '?';
  char sc = '?';
  for (int i = 0; i < cmd.length(); i++) {
    if (cmd[i] > 32) {
      c = lc(cmd[i]);
      if ((i < cmd.length() - 1) && (cmd[i + 1] > 32))
        sc = lc(cmd[i + 1]);
      break;
    }
  }
  switch (currState) {
  case ZCFGMENU_MAIN: {
    if ((c == 'q') || (cmd.length() == 0)) {
      if (settingsChanged) {
        currState = ZCFGMENU_WICONFIRM;
        showMenu = true;
      } else {
        commandMode.showInitMessage();
        switchBackToCommandMode();
        return;
      }
    } else if (c == 'a') // add to phonebook
    {
      currState = ZCFGMENU_NUM;
      showMenu = true;
    } else if (c == 'w') // wifi
    {
      currState = ZCFGMENU_WIMENU;
      showMenu = true;
    } else if (c == 'h') // host
    {
      currState = ZCFGMENU_NEWHOST;
      showMenu = true;
    } else if (c == 'f') // flow control
    {
      currState = ZCFGMENU_FLOW;
      showMenu = true;
    } else if ((c == 'p') && (sc == 'e')) // petscii translation toggle
    {
      commandMode.serial.setPetsciiMode(!commandMode.serial.isPetsciiMode());
      serial.setPetsciiMode(commandMode.serial.isPetsciiMode());
      settingsChanged = true;
      showMenu = true;
    } else if ((c == 'p') && (sc == 'r')) // print spec
    {
      currState = ZCFGMENU_NEWPRINT;
      showMenu = true;
    } else if (c == 'e') // echo
    {
      savedEcho = !savedEcho;
      settingsChanged = true;
      showMenu = true;
    } else if (c == 'b') // bbs
    {
      currState = ZCFGMENU_BBSMENU;
      showMenu = true;
    } else if (c == 'l') // language toggle
    {
      commandMode.isKorean = !commandMode.isKorean;
      settingsChanged = true;
      showMenu = true;
    } else if (c == 's') // speaker toggle
    {
      commandMode.speakerMode = (commandMode.speakerMode + 1) % 3;
      snd_speaker_mode = commandMode.speakerMode;
      settingsChanged = true;
      showMenu = true;
    } else if (c > 47 && c < 58) // its a phonebook entry!
    {
      PhoneBookEntry *pb = PhoneBookEntry::findPhonebookEntry(cmd);
      if (pb == null) {
        serial.printf("%s%sPhone number not found: '%s'.%s%s", EOLNC, EOLNC,
                      cmd.c_str(), EOLNC, EOLNC);
        currState = ZCFGMENU_MAIN;
        showMenu = true;
      } else {
        lastNumber = pb->number;
        lastAddress = pb->address;
        lastOptions = pb->modifiers;
        lastNotes = pb->notes;
        currState = ZCFGMENU_ADDRESS;
        showMenu = true;
      }
    } else {
      showMenu = true; // re-show the menu
    }
    break;
  }
  case ZCFGMENU_WICONFIRM: {
    if ((cmd.length() == 0) || (c == 'n')) {
      commandMode.showInitMessage();
      switchBackToCommandMode();
      return;
    } else if (c == 'y') {
      if (newListen != commandMode.preserveListeners) {
        commandMode.preserveListeners = newListen;
        if (!newListen) {
          SPIFFS.remove("/zlisteners.txt");
          WiFiServerNode::DestroyAllServers();
        } else {
          commandMode.ringCounter = 1;
          commandMode.autoStreamMode = true;
          WiFiServerNode *s = WiFiServerNode::FindServer(serverSpec.port);
          if (s != null)
            delete s;
          s = new WiFiServerNode(serverSpec.port, serverSpec.flagsBitmap);
          WiFiServerNode::SaveWiFiServers();
        }
      } else if (commandMode.preserveListeners) {
        WiFiServerNode *s = WiFiServerNode::FindServer(serverSpec.port);
        if (s != null) {
          if (s->flagsBitmap != serverSpec.flagsBitmap) {
            s->flagsBitmap = serverSpec.flagsBitmap;
          }
        } else {
          WiFiServerNode::DestroyAllServers();
          s = new WiFiServerNode(serverSpec.port, serverSpec.flagsBitmap);
          WiFiServerNode::SaveWiFiServers();
          commandMode.updateAutoAnswer();
        }
      }
      if (!commandMode.reSaveConfig(3))
        serial.printf("%sFailed to save Settings.%s", EOLNC, EOLNC);
      else
        serial.printf("%sSettings saved.%s", EOLNC, EOLNC);
      commandMode.showInitMessage();
      WiFiServerNode::SaveWiFiServers();
      switchBackToCommandMode();
      return;
    } else
      showMenu = true;
    break;
  }
  case ZCFGMENU_NUM: {
    PhoneBookEntry *pb = PhoneBookEntry::findPhonebookEntry(cmd);
    if (pb != null) {
      serial.printf("%s%sNumber already exists '%s'.%s%s", EOLNC, EOLNC,
                    cmd.c_str(), EOLNC, EOLNC);
      currState = ZCFGMENU_MAIN;
      showMenu = true;
    } else if (cmd.length() == 0) {
      currState = ZCFGMENU_MAIN;
      showMenu = true;
    } else {
      lastNumber = atol((char *)cmd.c_str());
      lastAddress = "";
      int flagsBitmap = commandMode.getConfigFlagBitmap();
      flagsBitmap = flagsBitmap & (~FLAG_ECHO);
      ConnSettings flags(flagsBitmap);
      lastOptions = flags.getFlagString();
      lastNotes = "";
      currState = ZCFGMENU_ADDRESS;
      showMenu = true;
    }
    break;
  }
  case ZCFGMENU_NEWPORT: {
    if (cmd.length() > 0) {
      serverSpec.port = atoi((char *)cmd.c_str());
      settingsChanged = true;
    }
    currState = ZCFGMENU_BBSMENU;
    showMenu = true;
    break;
  }
  case ZCFGMENU_ADDRESS: {
    PhoneBookEntry *entry = PhoneBookEntry::findPhonebookEntry(lastNumber);
    if (cmd.equalsIgnoreCase("delete") && (entry != null)) {
      delete entry;
      currState = ZCFGMENU_MAIN;
      serial.printf("%sPhonebook entry deleted.%s%s", EOLNC, EOLNC, EOLNC);
    } else if ((cmd.length() == 0) && (entry != null))
      currState = ZCFGMENU_NOTES; // just keep old values
    else {
      if (!validateHostInfo((uint8_t *)cmd.c_str()))
        serial.printf("%sInvalid address format (hostname:port) or "
                      "(user:pass@hostname:port) for '%s'.%s%s",
                      EOLNC, cmd.c_str(), EOLNC, EOLNC);
      else {
        lastAddress = cmd;
        currState = ZCFGMENU_NOTES;
        if (lastAddress.indexOf("@") > 0) {
          int x = lastOptions.indexOf("S");
          if (x < 0)
            lastOptions += "S";
          else
            lastOptions =
                lastOptions.substring(0, x) + lastOptions.substring(x + 1);
        }
      }
    }
    showMenu = true; // re-show the menu
    break;
  }
  case ZCFGMENU_BBSMENU: {
    if (cmd.length() == 0)
      currState = ZCFGMENU_MAIN;
    else {
      ConnSettings flags(serverSpec.flagsBitmap);
      switch (c) {
      case 'h':
        currState = ZCFGMENU_NEWPORT;
        break;
      case 'd':
        newListen = false;
        settingsChanged = true;
        break;
      case 'p':
        if (newListen) {
          flags.petscii = !flags.petscii;
          settingsChanged = true;
        }
        break;
      case 't':
        if (newListen) {
          flags.telnet = !flags.telnet;
          settingsChanged = true;
        }
        break;
      case 'e':
        if (newListen)
          flags.echo = !flags.echo;
        else
          newListen = true;
        settingsChanged = true;
        break;
      case 'f':
        if (newListen) {
          if (flags.xonxoff) {
            flags.xonxoff = false;
            flags.rtscts = true;
          } else if (flags.rtscts)
            flags.rtscts = false;
          else
            flags.xonxoff = true;
          settingsChanged = true;
        }
        break;
      case 's':
        if (newListen) {
          flags.secure = !flags.secure;
          settingsChanged = true;
        }
        break;

      default:
        serial.printf("%sInvalid option '%s'.%s%s", EOLNC, cmd.c_str(), EOLNC,
                      EOLNC);
        break;
      }
      settingsChanged = true;
      serverSpec.flagsBitmap = flags.getBitmap();
    }
    showMenu = true;
    break;
  }
  case ZCFGMENU_SUBNET: {
    if (cmd.length() == 0)
      currState = ZCFGMENU_NETMENU;
    else {
      IPAddress *newaddr = ConnSettings::parseIP(cmd.c_str());
      if (newaddr == null)
        serial.printf("%sBad address (%s).%s", EOLNC, cmd.c_str(), EOLNC);
      else {
        currState = ZCFGMENU_NETMENU;
        switch (netOpt) {
        case 'i':
          lastIP = *newaddr;
          break;
        case 'd':
          lastDNS = *newaddr;
          break;
        case 'g':
          lastGW = *newaddr;
          break;
        case 's':
          lastSN = *newaddr;
          break;
        }
        free(newaddr);
      }
    }
    showMenu = true;
    break;
  }
  case ZCFGMENU_NETMENU: {
    if (cmd.length() == 0) {
      currState = ZCFGMENU_MAIN;
      if ((staticIP == null) && (useDHCP)) {
      } else if (useDHCP) {
        setNewStaticIPs(null, null, null, null);
        if (!connectWifi(wifiSSI.c_str(), wifiPW.c_str(), staticIP, staticDNS,
                         staticGW, staticSN))
          serial.printf("%sUnable to connect to %s. :(%s", EOLNC,
                        wifiSSI.c_str(), EOLNC);
        settingsChanged = true;
      } else if ((staticIP == null) || (*staticIP != lastIP) ||
                 (*staticDNS != lastDNS) || (*staticGW != lastGW) ||
                 (*staticSN != lastSN)) {
        IPAddress *ip = new IPAddress();
        *ip = lastIP;
        IPAddress *dns = new IPAddress();
        *dns = lastDNS;
        IPAddress *gw = new IPAddress();
        *gw = lastGW;
        IPAddress *sn = new IPAddress();
        *sn = lastSN;
        setNewStaticIPs(ip, dns, gw, sn);
        if (!connectWifi(wifiSSI.c_str(), wifiPW.c_str(), staticIP, staticDNS,
                         staticGW, staticSN))
          serial.printf("%sUnable to connect to %s. :(%s", EOLNC,
                        wifiSSI.c_str(), EOLNC);
        settingsChanged = true;
      }
    } else if (useDHCP) {
      if (c == 'd')
        useDHCP = false;
    } else {
      switch (c) {
      case 'e':
        useDHCP = true;
        break;
      case 'i':
      case 'd':
      case 'g':
      case 's':
        netOpt = c;
        currState = ZCFGMENU_SUBNET;
        break;
      default:
        serial.printf("%sInvalid option '%s'.%s%s", EOLNC, cmd.c_str(), EOLNC,
                      EOLNC);
        break;
      }
    }
    showMenu = true;
    break;
  }
  case ZCFGMENU_NOTES: {
    if (cmd.length() > 0)
      lastNotes = cmd;
    currState = ZCFGMENU_OPTIONS;
    showMenu = true; // re-show the menu
    break;
  }
  case ZCFGMENU_OPTIONS: {
    if (cmd.length() == 0) {
      PhoneBookEntry *entry = PhoneBookEntry::findPhonebookEntry(lastNumber);
      if (entry != null) {
        serial.printf("%sPhonebook entry updated.%s%s", EOLNC, EOLNC, EOLNC);
        delete entry;
      } else
        serial.printf("%sPhonebook entry added.%s%s", EOLNC, EOLNC, EOLNC);
      entry = new PhoneBookEntry(lastNumber, lastAddress.c_str(),
                                 lastOptions.c_str(), lastNotes.c_str());
      PhoneBookEntry::savePhonebook();
      currState = ZCFGMENU_MAIN;
    } else {
      ConnSettings flags(lastOptions.c_str());
      switch (c) {
      case 'p':
        flags.petscii = !flags.petscii;
        break;
      case 't':
        flags.telnet = !flags.telnet;
        break;
      case 'e':
        flags.echo = !flags.echo;
        break;
      case 'f':
        if (flags.xonxoff) {
          flags.xonxoff = false;
          flags.rtscts = true;
        } else if (flags.rtscts)
          flags.rtscts = false;
        else
          flags.xonxoff = true;
        break;
      case 's':
        flags.secure = !flags.secure;
        break;
      default:
        serial.printf("%sInvalid toggle option '%s'.%s%s", EOLNC, cmd.c_str(),
                      EOLNC, EOLNC);
        break;
      }
      lastOptions = flags.getFlagString();
    }
    showMenu = true; // re-show the menu
    break;
  }
  case ZCFGMENU_WIMENU: {
    if (cmd.length() == 0) {
      currState = ZCFGMENU_MAIN;
      showMenu = true;
    } else {
      int num = atoi(cmd.c_str());
      if ((num <= 0) || (num > lastNumNetworks))
        serial.printf("%sInvalid number.  Try again.%s", EOLNC, EOLNC);
      else if (WiFi.encryptionType(num - 1) == ENC_TYPE_NONE) {
        if (!connectWifi(WiFi.SSID(num - 1).c_str(), "", null, null, null,
                         null)) {
          serial.printf("%sUnable to connect to %s. :(%s", EOLNC,
                        WiFi.SSID(num - 1).c_str(), EOLNC);
        } else {
          wifiSSI = WiFi.SSID(num - 1);
          wifiPW = "";
          settingsChanged = true;
          serial.printf("%sConnected!%s", EOLNC, EOLNC);
          currState = ZCFGMENU_NETMENU;
        }
        showMenu = true;
      } else {
        lastNumber = num - 1;
        currState = ZCFGMENU_WIFIPW;
        showMenu = true;
      }
    }
    break;
  }
  case ZCFGMENU_NEWHOST:
    if (cmd.length() == 0)
      currState = ZCFGMENU_WIMENU;
    else {
      hostname = cmd;
      hostname.replace(',', '.');
      if ((wifiSSI.length() > 0) && (WiFi.status() == WL_CONNECTED)) {
        if (!connectWifi(wifiSSI.c_str(), wifiPW.c_str(), staticIP, staticDNS,
                         staticGW, staticSN))
          serial.printf("%sUnable to connect to %s. :(%s", EOLNC,
                        wifiSSI.c_str(), EOLNC);
        settingsChanged = true;
      }
      currState = ZCFGMENU_MAIN;
      showMenu = true;
    }
    break;
  case ZCFGMENU_NEWPRINT:
    if (cmd.length() > 0) {
      if (!printMode.testPrinterSpec(cmd.c_str(), cmd.length(),
                                     commandMode.serial.isPetsciiMode())) {
        serial.printf("%sBad format. Try ?:<host>:<port>/<path>.%s", EOLNC,
                      EOLNC);
        serial.printf("? = A)scii P)etscii or R)aw.%s", EOLNC);
        serial.printf("Example: R:192.168.1.71:631/ipp/printer%s", EOLNC);
      } else {
        printMode.setLastPrinterSpec(cmd.c_str());
        settingsChanged = true;
      }
    }
    currState = ZCFGMENU_MAIN;
    showMenu = true;
    break;
  case ZCFGMENU_WIFIPW:
    if (cmd.length() == 0) {
      currState = ZCFGMENU_WIMENU;
      showMenu = true;
    } else {
      for (int i = 0; i < 500; i++) {
        if (serial.isSerialOut())
          serialOutDeque();
        delay(1);
      }
      if (!connectWifi(WiFi.SSID(lastNumber).c_str(), cmd.c_str(), null, null,
                       null, null))
        serial.printf("%sUnable to connect to %s.%s", EOLNC,
                      WiFi.SSID(lastNumber).c_str(), EOLNC);
      else {
        // setNewStaticIPs(null,null,null,null);
        useDHCP = (staticIP == null);
        lastIP = (staticIP != null) ? *staticIP : WiFi.localIP();
        lastDNS = (staticDNS != null) ? *staticDNS : IPAddress(192, 168, 0, 1);
        lastGW = (staticGW != null) ? *staticGW : IPAddress(192, 168, 0, 1);
        lastSN = (staticSN != null) ? *staticSN : IPAddress(255, 255, 255, 0);
        wifiSSI = WiFi.SSID(lastNumber);
        wifiPW = cmd;
        settingsChanged = true;
        currState = ZCFGMENU_NETMENU;
      }
      showMenu = true;
    }
    break;
  case ZCFGMENU_FLOW:
    if (cmd.length() == 0) {
      currState = ZCFGMENU_WIMENU;
      showMenu = true;
    } else {
      currState = ZCFGMENU_MAIN;
      showMenu = true;
      if (c == 'x')
        commandMode.serial.setFlowControlType(FCT_NORMAL);
      else if (c == 'r')
        commandMode.serial.setFlowControlType(FCT_RTSCTS);
      else if (c == 'd')
        commandMode.serial.setFlowControlType(FCT_DISABLED);
      else {
        serial.printf("%sUnknown flow control type '%s'.  Try again.%s", EOLNC,
                      cmd.c_str(), EOLNC);
        currState = ZCFGMENU_FLOW;
      }
      settingsChanged = settingsChanged || (currState == ZCFGMENU_MAIN);
      serial.setFlowControlType(commandMode.serial.getFlowControlType());
      serial.setXON(true);
    }
    break;
  }
}

void ZConfig::loop() {
  if (showMenu) {
    showMenu = false;
    switch (currState) {
    case ZCFGMENU_MAIN: {
      // ── VT100/ANSI 색상 (Bright Blue BBS 테마) ──────────────
      const char *BD = "\033[1;36;44m"; // Bright Cyan on Blue  - 테두리/배경
      const char *TL = "\033[1;37;44m"; // Bright White on Blue - 제목
      const char *SM = "\033[1;33;44m"; // Bright Yellow on Blue - 서브제목/라벨
      const char *VL = "\033[0;37;44m"; // White on Blue         - 값/텍스트
      const char *RS = "\033[0m";       // Reset

      // ── EUC-KR 한글 선문자 (새롬 데이타맨용, 1글자당 2컬럼 차지) ──
      const char *BOX_TL = "\xA6\xA3"; // ┌
      const char *BOX_TR = "\xA6\xA4"; // ┐
      const char *BOX_BL = "\xA6\xA6"; // └
      const char *BOX_BR = "\xA6\xA5"; // ┘
      const char *BOX_VL = "\xA6\xA2"; // │
      const char *BOX_HL = "\xA6\xA1"; // ─
      const char *BOX_ML = "\xA6\xA7"; // ├
      const char *BOX_MR = "\xA6\xA9"; // ┤

      // ── 흐름제어/BBS 모드 문자열 계산 ─────────────────────────
      String flowName;
      switch (commandMode.serial.getFlowControlType()) {
      case FCT_NORMAL:
        flowName = "XON/XOFF";
        break;
      case FCT_RTSCTS:
        flowName = "RTS/CTS";
        break;
      case FCT_DISABLED:
        flowName = "DISABLED";
        break;
      default:
        flowName = "OTHER";
        break;
      }
      String bbsMode = "DISABLED";
      if (newListen) {
        bbsMode = "Port ";
        bbsMode += serverSpec.port;
      }

      // 내부 너비 계산 (총 78컬럼 = 좌측선 2칸 + 내부 74칸 + 우측선 2칸)
      const int IW = 74;
      const int HL_COUNT = IW / 2; // 가로선(─)은 2칸을 차지하므로 37번만 반복

      // 화면 지우기 + 커서 홈
      serial.prints("\033[2J\033[H");

      // ┌──────────────────────────────────────┐
      serial.prints(BD);
      serial.prints(BOX_TL);
      for (int i = 0; i < HL_COUNT; i++)
        serial.prints(BOX_HL);
      serial.prints(BOX_TR);
      serial.prints(EOLNC);

      bool k = commandMode.isKorean;

      // │ AirLink 56K Setting          Main Menu │
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(TL);
      if (k) {
        serial.prints(" AirLink 56K \xBC\xB3\xC1\xA4  "); // 19 byte
        serial.prints(BD);
        for (int i = 0; i < 44; i++)
          serial.prints(" "); // 패딩
        serial.prints(SM);
        serial.prints(" \xB8\xDE\xC0\xCE \xB8\xDE\xB4\xBA "); // 10 byte
      } else {
        serial.prints(" AirLink 56K Setting");
        serial.prints(BD);
        for (int i = 0; i < 44; i++)
          serial.prints(" ");
        serial.prints(SM);
        serial.prints("Main Menu ");
      }
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(EOLNC);

      // ├──────────────────────────────────────┤
      serial.prints(BD);
      serial.prints(BOX_ML);
      for (int i = 0; i < HL_COUNT; i++)
        serial.prints(BOX_HL);
      serial.prints(BOX_MR);
      serial.prints(EOLNC);

      // 빈 줄
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(VL);
      for (int i = 0; i < IW; i++)
        serial.prints(" ");
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(EOLNC);

      // ── 메뉴 항목 출력 람다 ────────────────────────────────────
      auto menuLine = [&](const char *lbl, const char *desc, const char *val) {
        char vis[128];
        snprintf(vis, sizeof(vis), "  %-10s  %s%s", lbl, desc, val);
        // EUC-KR 문자는 1글자당 2바이트를 차지하므로, 문자열 길이(strlen)가 곧
        // 터미널의 표시 너비(2칸)와 대략 일치합니다.
        int visLen = strlen(vis);
        int pad = IW - visLen;
        if (pad < 0)
          pad = 0;

        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(SM);
        serial.printf("  %-10s", lbl);
        serial.prints(VL);
        serial.printf("  %s", desc);
        serial.prints(TL);
        serial.prints(val);
        serial.prints(VL);
        for (int i = 0; i < pad; i++)
          serial.prints(" ");
        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(EOLNC);
      };

      menuLine("[HOST]", k ? "\xC8\xA3\xBD\xBA\xC6\xAE\xB8\xED: " : "name: ",
               hostname.c_str());
      menuLine(
          "[WIFI]",
          k ? "\xBF\xCD\xC0\xCC\xC6\xC4\xC0\xCC \xBB\xF3\xC5\xC2: "
            : "connection: ",
          (WiFi.status() == WL_CONNECTED)
              ? wifiSSI.c_str()
              : (k ? "\xBF\xAC\xB0\xE1 \xBE\xC8\xB5\xCA" : "Not connected"));
      menuLine("[FLOW]",
               k ? "\xC8\xE5\xB8\xA7 \xC1\xA6\xBE\xEE: " : "control: ",
               flowName.c_str());
      menuLine("[ECHO]",
               k ? "\xB8\xED\xB7\xC9\xBE\xEE \xBF\xA1\xC4\xDA: "
                 : "keystrokes: ",
               savedEcho ? "ON" : "OFF");
      menuLine("[BBS]",
               k ? "\xBC\xF6\xBD\xC5 \xBC\xAD\xB9\xF6 \xC6\xF7\xC6\xAE: "
                 : "host: ",
               bbsMode.c_str());
      menuLine("[PRINT]", k ? "\xC7\xC1\xB8\xB0\xC5\xCD: " : "spec: ",
               printMode.getLastPrinterSpec());
      menuLine("[PETSCII]", k ? "PETSCII \xBA\xAF\xC8\xAF: " : "translation: ",
               commandMode.serial.isPetsciiMode() ? "ON" : "OFF");
      menuLine("[LANG]", k ? "\xBE\xF0\xBE\xEE(Language): " : "language: ",
               k ? "\xC7\xD1\xB1\xB9\xBE\xEE(KOREAN)" : "ENGLISH");
      menuLine("[SPEAKER]", k ? "\xBD\xBA\xC7\xC7\xC4\xBF(ATM): " : "speaker(ATM): ",
               commandMode.speakerMode == 0 ? (k ? "\xB2\xFB (0)" : "OFF (0)")
               : commandMode.speakerMode == 1 ? (k ? "\xB4\xD9\xC0\xCC\xBE\xF3\xBD\xC3 (1)" : "DIAL (1)")
                                              : (k ? "\xC7\xD7\xBB\xF3\xC4\xD4 (2)" : "ON (2)"));
      menuLine("[ADD]",
               k ? "\xBB\xF5 \xC0\xFC\xC8\xAD\xB9\xF8\xC8\xA3\xBA\xCE "
                   "\xC3\xDF\xB0\xA1"
                 : "new phonebook entry",
               "");

      // 전화번호부 항목
      PhoneBookEntry *p = phonebook;
      if (p != null) {
        // 구분 빈줄
        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(VL);
        for (int i = 0; i < IW; i++)
          serial.prints(" ");
        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(EOLNC);

        // "Phonebook entries:" 헤더
        const char *pbHdr = k ? "  \xC0\xFA\xC0\xE5\xB5\xC8 "
                                "\xC0\xFC\xC8\xAD\xB9\xF8\xC8\xA3\xBA\xCE:"
                              : "  Phonebook entries:";
        int phLen = strlen(pbHdr);
        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(SM);
        serial.prints(pbHdr);
        serial.prints(VL);
        for (int i = 0; i < IW - phLen; i++)
          serial.prints(" ");
        serial.prints(BD);
        serial.prints(BOX_VL);
        serial.prints(EOLNC);

        while (p != null) {
          char pbEntry[128];
          if (strlen(p->notes) > 0)
            snprintf(pbEntry, sizeof(pbEntry), "    [%lu] %s (%s)", p->number,
                     p->address, p->notes);
          else
            snprintf(pbEntry, sizeof(pbEntry), "    [%lu] %s", p->number,
                     p->address);
          int eLen = strlen(pbEntry);
          int ePad = IW - eLen;
          if (ePad < 0)
            ePad = 0;
          serial.prints(BD);
          serial.prints(BOX_VL);
          serial.prints(VL);
          serial.prints(pbEntry);
          for (int i = 0; i < ePad; i++)
            serial.prints(" ");
          serial.prints(BD);
          serial.prints(BOX_VL);
          serial.prints(EOLNC);
          p = p->next;
        }
      }

      // 빈 줄
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(VL);
      for (int i = 0; i < IW; i++)
        serial.prints(" ");
      serial.prints(BD);
      serial.prints(BOX_VL);
      serial.prints(EOLNC);

      // └──────────────────────────────────────┘
      serial.prints(BD);
      serial.prints(BOX_BL);
      for (int i = 0; i < HL_COUNT; i++)
        serial.prints(BOX_HL);
      serial.prints(BOX_BR);
      serial.prints(EOLNC);

      // 프롬프트 (박스 아래, 검정 배경)
      serial.prints("\033[1;30;40m"); // Dark Gray on Black
      serial.prints(EOLNC);
      serial.prints(k ? "  \xA1\xD8 \xBE\xC8\xB3\xBB: \xB8\xED\xB7\xC9\xBE\xEE\xB4\xC2 \xBE\xD5\xB1\xDB\xC0\xDA \xC7\xCF\xB3\xAA\xB8\xB8 \xC0\xD4\xB7\xC2\xC7\xCF\xBC\xC5\xB5\xB5 \xB5\xCB\xB4\xCF\xB4\xD9."
                      : "  * Note: You only need to type the first letter of the command.");
      serial.prints(EOLNC);
      serial.prints(EOLNC); // 빈 줄 추가
      serial.prints("\033[0;33;40m"); // Yellow on Black
      serial.prints(k ? "  \xB8\xED\xB7\xC9\xBE\xEE \xC0\xD4\xB7\xC2 "
                        "(\xC1\xBE\xB7\xE1\xC7\xCF\xB7\xC1\xB8\xE9 ENTER): "
                      : "  Enter command or ENTER to exit: ");
      serial.prints("\033[1;37;40m"); // Bright White
      serial.prints(RS);
      break;
    }

    case ZCFGMENU_NUM:
      serial.printf(
          commandMode.isKorean
              ? "%s\xBB\xF5 \xB0\xA1\xBB\xF3 \xC0\xFC\xC8\xAD\xB9\xF8\xC8\xA3 "
                "\xC0\xD4\xB7\xC2 (\xBC\xFD\xC0\xDA\xB8\xB8)%s: "
              : "%sEnter a new fake phone number (digits ONLY)%s: ",
          EOLNC, EOLNC);
      break;
    case ZCFGMENU_NEWPORT:
      serial.printf(
          commandMode.isKorean
              ? "%s\xBC\xF6\xBD\xC5 \xB4\xEB\xB1\xE2\xC7\xD2 \xC6\xF7\xC6\xAE "
                "\xB9\xF8\xC8\xA3 \xC0\xD4\xB7\xC2%s: "
              : "%sEnter a port number to listen on%s: ",
          EOLNC, EOLNC);
      break;
    case ZCFGMENU_ADDRESS: {
      PhoneBookEntry *lastEntry =
          PhoneBookEntry::findPhonebookEntry(lastNumber);
      if (lastEntry == null)
        serial.printf(
            commandMode.isKorean
                ? "%s\xC8\xA3\xBD\xBA\xC6\xAE\xB8\xED:\xC6\xF7\xC6\xAE "
                  "\xC0\xD4\xB7\xC2, SSH\xB4\xC2 "
                  "user:pass@\xC8\xA3\xBD\xBA\xC6\xAE\xB8\xED:\xC6\xF7\xC6\xAE "
                  "\xC0\xD4\xB7\xC2%s: "
                : "%sEnter hostname:port, or user:pass@hostname:port for "
                  "SSH%s: ",
            EOLNC, EOLNC);
      else
        serial.printf(
            commandMode.isKorean
                ? "%s\xC1\xD6\xBC\xD2 \xBC\xF6\xC1\xA4, \xB6\xC7\xB4\xC2 "
                  "DELETE \xC0\xD4\xB7\xC2\xBD\xC3 \xBB\xE8\xC1\xA6 (%s)%s: "
                : "%sModify address, or enter DELETE (%s)%s: ",
            EOLNC, lastAddress.c_str(), EOLNC);
      break;
    }
    case ZCFGMENU_OPTIONS: {
      ConnSettings flags(lastOptions.c_str());
      serial.printf(commandMode.isKorean
                        ? "%s\xBF\xAC\xB0\xE1 \xBF\xC9\xBC\xC7:%s"
                        : "%sConnection Options:%s",
                    EOLNC, EOLNC);
      serial.printf(commandMode.isKorean ? "[PETSCII] \xBA\xAF\xC8\xAF: %s%s"
                                         : "[PETSCII] Translation: %s%s",
                    flags.petscii ? "ON" : "OFF", EOLNC);
      serial.printf(commandMode.isKorean ? "[TELNET] \xBA\xAF\xC8\xAF: %s%s"
                                         : "[TELNET] Translation: %s%s",
                    flags.telnet ? "ON" : "OFF", EOLNC);
      serial.printf(commandMode.isKorean ? "[ECHO] \xBF\xA1\xC4\xDA: %s%s"
                                         : "[ECHO]: %s%s",
                    flags.echo ? "ON" : "OFF", EOLNC);
      serial.printf(commandMode.isKorean
                        ? "[FLOW] \xC8\xE5\xB8\xA7 \xC1\xA6\xBE\xEE: %s%s"
                        : "[FLOW] Control: %s%s",
                    flags.xonxoff  ? "XON/XOFF"
                    : flags.rtscts ? "RTS/CTS"
                                   : "DISABLED",
                    EOLNC);
      serial.printf(commandMode.isKorean ? "[SSH/SSL] \xBA\xB8\xBE\xC8: %s%s"
                                         : "[SSH/SSL] Security: %s%s",
                    flags.secure ? "ON" : "OFF", EOLNC);
      serial.printf(commandMode.isKorean
                        ? "%s\xBA\xAF\xB0\xE6\xC7\xD2 \xBF\xC9\xBC\xC7 "
                          "\xB4\xDC\xC3\xE0\xC5\xB0 \xC0\xD4\xB7\xC2, "
                          "\xB6\xC7\xB4\xC2 ENTER\xB7\xCE \xC1\xBE\xB7\xE1%s: "
                        : "%sEnter option to toggle or ENTER to exit%s: ",
                    EOLNC, EOLNC);
      break;
    }
    case ZCFGMENU_NOTES: {
      serial.printf(
          commandMode.isKorean
              ? "%s\xC0\xCC \xC7\xD7\xB8\xF1\xBF\xA1 \xB4\xEB\xC7\xD1 "
                "\xB8\xDE\xB8\xF0 \xC0\xD4\xB7\xC2 (%s)%s: "
              : "%sEnter some notes for this entry (%s)%s: ",
          EOLNC, lastNotes.c_str(), EOLNC);
      break;
    }
    case ZCFGMENU_BBSMENU: {
      serial.printf(
          commandMode.isKorean
              ? "%sBBS \xBC\xF6\xBD\xC5 \xBC\xAD\xB9\xF6 \xBC\xB3\xC1\xA4:%s"
              : "%sBBS host settings:%s",
          EOLNC, EOLNC);
      if (newListen) {
        ConnSettings flags(serverSpec.flagsBitmap);
        serial.printf(commandMode.isKorean
                          ? "%s[HOST] \xBC\xF6\xBD\xC5 \xC6\xF7\xC6\xAE: %d%s"
                          : "%s[HOST] Listener Port: %d%s",
                      EOLNC, serverSpec.port, EOLNC);
        serial.printf(commandMode.isKorean ? "[PETSCII] \xBA\xAF\xC8\xAF: %s%s"
                                           : "[PETSCII] Translation: %s%s",
                      flags.petscii ? "ON" : "OFF", EOLNC);
        serial.printf(commandMode.isKorean ? "[TELNET] \xBA\xAF\xC8\xAF: %s%s"
                                           : "[TELNET] Translation: %s%s",
                      flags.telnet ? "ON" : "OFF", EOLNC);
        serial.printf(commandMode.isKorean ? "[ECHO] \xBF\xA1\xC4\xDA: %s%s"
                                           : "[ECHO]: %s%s",
                      flags.echo ? "ON" : "OFF", EOLNC);
        serial.printf(commandMode.isKorean
                          ? "[FLOW] \xC8\xE5\xB8\xA7 \xC1\xA6\xBE\xEE: %s%s"
                          : "[FLOW] Control: %s%s",
                      flags.xonxoff  ? "XON/XOFF"
                      : flags.rtscts ? "RTS/CTS"
                                     : "DISABLED",
                      EOLNC);
        serial.printf(commandMode.isKorean
                          ? "[DISABLE] BBS \xC8\xA3\xBD\xBA\xC6\xAE "
                            "\xBC\xF6\xBD\xC5 \xB2\xF4\xB1\xE2%s"
                          : "[DISABLE] BBS host listener%s",
                      EOLNC);
      } else
        serial.printf(commandMode.isKorean
                          ? "%s[ENABLE] BBS \xC8\xA3\xBD\xBA\xC6\xAE "
                            "\xBC\xF6\xBD\xC5 \xC4\xD1\xB1\xE2%s"
                          : "%s[ENABLE] BBS host listener%s",
                      EOLNC, EOLNC);
      serial.printf(commandMode.isKorean
                        ? "%s\xBA\xAF\xB0\xE6\xC7\xD2 \xBF\xC9\xBC\xC7 "
                          "\xB4\xDC\xC3\xE0\xC5\xB0 \xC0\xD4\xB7\xC2, "
                          "\xB6\xC7\xB4\xC2 ENTER\xB7\xCE \xC1\xBE\xB7\xE1%s: "
                        : "%sEnter option to toggle or ENTER to exit%s: ",
                    EOLNC, EOLNC);
      break;
    }
    case ZCFGMENU_SUBNET: {
      String str;
      switch (netOpt) {
      case 'i': {
        ConnSettings::IPtoStr(&lastIP, str);
        serial.printf(commandMode.isKorean ? "%sIP \xC1\xD6\xBC\xD2 (%s)%s: %s" : "%sIP Address (%s)%s: %s", EOLNC, str.c_str(), EOLNC, EOLNC);
        break;
      }
      case 'g': {
        ConnSettings::IPtoStr(&lastGW, str);
        serial.printf(commandMode.isKorean ? "%s\xB0\xD4\xC0\xCC\xC6\xAE\xBF\xFE\xC0\xCC \xC1\xD6\xBC\xD2 (%s)%s: %s" : "%sGateway Address (%s)%s: %s", EOLNC, str.c_str(), EOLNC, EOLNC);
        break;
      }
      case 's': {
        ConnSettings::IPtoStr(&lastSN, str);
        serial.printf(commandMode.isKorean ? "%s\xBC\xAD\xBA\xEA\xB3\xDD \xB8\xB6\xBD\xBA\xC5\xA9 (%s)%s: %s" : "%sSubnet Mask Address (%s)%s: %s", EOLNC, str.c_str(), EOLNC, EOLNC);
        break;
      }
      case 'd': {
        ConnSettings::IPtoStr(&lastDNS, str);
        serial.printf(commandMode.isKorean ? "%sDNS \xC1\xD6\xBC\xD2 (%s)%s: %s" : "%sDNS Address (%s)%s: %s", EOLNC, str.c_str(), EOLNC, EOLNC);
        break;
      }
      }
      break;
    }
    case ZCFGMENU_NETMENU: {
      serial.printf(commandMode.isKorean ? "%s\xB3\xD7\xC6\xAE\xBF\xF6\xC5\xA9 \xBC\xB3\xC1\xA4:%s" : "%sNetwork settings:%s", EOLNC, EOLNC);
      if (!useDHCP) {
        String str;
        ConnSettings::IPtoStr(&lastIP, str);
        serial.printf("%s[IP]: %s%s", EOLNC, str.c_str(), EOLNC);
        ConnSettings::IPtoStr(&lastSN, str);
        serial.printf(commandMode.isKorean ? "[\xBC\xAD\xBA\xEA\xB3\xDD]: %s%s" : "[SUBNET]: %s%s", str.c_str(), EOLNC);
        ConnSettings::IPtoStr(&lastGW, str);
        serial.printf(commandMode.isKorean ? "[\xB0\xD4\xC0\xCC\xC6\xAE\xBF\xFE\xC0\xCC]: %s%s" : "[GATEWAY]: %s%s", str.c_str(), EOLNC);
        ConnSettings::IPtoStr(&lastDNS, str);
        serial.printf("[DNS]: %s%s", str.c_str(), EOLNC);
        serial.printf(commandMode.isKorean ? "[\xC8\xB0\xBC\xBA\xC8\xAD] DHCP \xBB\xE7\xBF\xEB%s" : "[ENABLE] Enable DHCP%s", EOLNC);
      } else
        serial.printf(commandMode.isKorean ? "%s[\xBA\xF1\xC8\xB0\xBC\xBA\xC8\xAD] DHCP \xBB\xE7\xBF\xEB \xBE\xC8 \xC7\xD4%s" : "%s[DISABLE] DHCP%s", EOLNC, EOLNC);
      serial.printf(commandMode.isKorean ? "%s\xBA\xAF\xB0\xE6\xC7\xD2 \xBF\xC9\xBC\xC7 \xB4\xDC\xC3\xE0\xC5\xB0 \xC0\xD4\xB7\xC2, \xB6\xC7\xB4\xC2 ENTER\xB7\xCE \xC1\xBE\xB7\xE1%s: " : "%sEnter option to toggle or ENTER to exit%s: ", EOLNC, EOLNC);
      break;
    }
    case ZCFGMENU_WIMENU: {
      int n = WiFi.scanNetworks();
      if (n > 20)
        n = 20;
      serial.printf(commandMode.isKorean ? "%s\xBF\xCD\xC0\xCC\xC6\xC4\xC0\xCC \xB8\xF1\xB7\xCF:%s" : "%sWiFi Networks:%s", EOLNC, EOLNC);
      lastNumNetworks = n;
      for (int i = 0; i < n; ++i) {
        serial.printf("[%d] ", (i + 1));
        serial.prints(WiFi.SSID(i).c_str());
        serial.prints(" (");
        serial.printi(WiFi.RSSI(i));
        serial.prints(")");
        serial.prints((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        serial.prints(EOLN.c_str());
        serial.flush();
        delay(10);
      }
      serial.printf(commandMode.isKorean ? "%s\xBF\xAC\xB0\xE1\xC7\xD2 \xB9\xF8\xC8\xA3 \xC0\xD4\xB7\xC2 (\xC1\xBE\xB7\xE1\xB4\xC2 ENTER): " : "%sEnter number to connect, or ENTER: ", EOLNC);
      break;
    }
    case ZCFGMENU_NEWHOST: {
      serial.printf(commandMode.isKorean ? "%s\xBB\xF5 \xC8\xA3\xBD\xBA\xC6\xAE\xB8\xED \xC0\xD4\xB7\xC2: " : "%sEnter a new hostname: ", EOLNC);
      break;
    }
    case ZCFGMENU_NEWPRINT: {
      serial.printf(
          commandMode.isKorean ? "%sIPP \xC7\xC1\xB8\xB0\xC5\xCD \xC1\xD6\xBC\xD2 \xC0\xD4\xB7\xC2 (?:<host>:<port>/path)%s: " : "%sEnter ipp printer spec (?:<host>:<port>/path)%s: ", EOLNC, EOLNC);
      break;
    }
    case ZCFGMENU_WIFIPW: {
      serial.printf(commandMode.isKorean ? "%s\xBF\xCD\xC0\xCC\xC6\xC4\xC0\xCC \xBA\xF1\xB9\xD0\xB9\xF8\xC8\xA3 \xC0\xD4\xB7\xC2: " : "%sEnter your WiFi Password: ", EOLNC);
      break;
    }
    case ZCFGMENU_FLOW: {
      serial.printf(commandMode.isKorean ? "%sRTS/CTS, XON/XOFF, \xB6\xC7\xB4\xC2 DISABLE \xC0\xD4\xB7\xC2%s: " : "%sEnter RTS/CTS, XON/XOFF, or DISABLE flow control%s: ",
                    EOLNC, EOLNC);
      break;
    }
    case ZCFGMENU_WICONFIRM: {
      serial.printf(commandMode.isKorean ? "%s\xBC\xB3\xC1\xA4\xC0\xCC \xBA\xAF\xB0\xE6\xB5\xC7\xBE\xFA\xBD\xC0\xB4\xCF\xB4\xD9. \xC0\xFA\xC0\xE5\xC7\xCF\xBD\xC3\xB0\xDA\xBD\xC0\xB4\xCF\xB1\xEE (y/N)?" : "%sYour settings changed. Save (y/N)?", EOLNC);
      break;
    }
    }
  }
  if (checkPlusPlusPlusEscape()) {
    switchBackToCommandMode();
  } else if (serial.isSerialOut()) {
    serialOutDeque();
  }
  logFileLoop();
}
