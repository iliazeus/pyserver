# pysession

A persistent Python session accessible from the shell.

```
$ pysession &
[1] 82939
$ py -c 'import json'
$ curl -s https://api.github.com/repos/python/cpython | py -C 'repo = json.load(stdin)'
$ curl -s "$(py -p 'repo["owner"]["avatar_url"]')" -o avatar.png
$ file avatar.png
avatar.png: PNG image data, 460 x 460, 8-bit/color RGBA, non-interlaced
```
