import urllib.request
url = "https://raw.githubusercontent.com/ppp-project/ppp/master/pppd/upap.c"
req = urllib.request.urlopen(url)
data = req.read().decode('utf-8')
lines = data.split('\n')
for i, line in enumerate(lines):
    if "upap_sresp" in line or "UPAP_AUTHACK" in line:
        print(f"{i}: {line}")
