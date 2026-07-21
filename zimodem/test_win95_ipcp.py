import urllib.request
url = "https://raw.githubusercontent.com/ppp-project/ppp/master/pppd/ipcp.c"
req = urllib.request.urlopen(url)
data = req.read().decode('utf-8')
lines = data.split('\n')
for i, line in enumerate(lines):
    if "ipcp_reqci" in line:
        print(f"{i}: {line}")
