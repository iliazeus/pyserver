# pyserver

A persistent Python session accessible from the shell.

```
$ pyserver &
[1] 82939
$ py -c 'import json'
$ curl https://api.github.com/repos/python/cpython | py -C 'repo = json.load(stdin)'
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  6546  100  6546    0     0   8979      0 --:--:-- --:--:-- --:--:--  8979
$ curl "$(py -p 'repo["owner"]["avatar_url"]')" -o avatar.png
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 25109  100 25109    0     0  62399      0 --:--:-- --:--:-- --:--:-- 62460
$ file avatar.png
avatar.png: PNG image data, 460 x 460, 8-bit/color RGBA, non-interlaced
```
