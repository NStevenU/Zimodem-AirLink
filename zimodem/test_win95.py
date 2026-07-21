import urllib.request
url = "https://raw.githubusercontent.com/ppp-project/ppp/master/pppd/auth.c"
req = urllib.request.urlopen(url)
data = req.read().decode('utf-8')
lines = data.split('\n')
for i, line in enumerate(lines):
    if "PAP" in line and "auth" in line.lower():
        print(f"{i}: {line}")
