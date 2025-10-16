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

```
$ py -h
py - pysession client (https://github.com/iliazeus/pysession)
Usage: py -c <code> [...args]  - run code
       py -C <code> [...args]  - run code with stdin
       py -p [...args]         - print args
       py -i [...args]         - interactive mode
       py | py -h              - this text
```
