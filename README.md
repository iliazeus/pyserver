# pysession

A persistent Python session accessible from the shell.

```
$ pysession &
[1] 82939
$ py -c 'import json'
$ curl -s https://api.github.com/repos/python/cpython | py -c 'repo = json.load(stdin)'
$ curl -s "$(py -p 'repo["owner"]["avatar_url"]')" -o avatar.png
$ file avatar.png
avatar.png: PNG image data, 460 x 460, 8-bit/color RGBA, non-interlaced
```

```
$ py --help
usage: py [-h] [-i] [-c [COMMAND ...]] [-p [PRINT ...]] [argv ...]

pysession client (https://github.com/iliazeus/pysession)

positional arguments:
  argv

options:
  -h, --help            show this help message and exit
  -i, --interactive     interactive mode
  -c [COMMAND ...], --command [COMMAND ...]
                        run code passed via args
  -p [PRINT ...], --print [PRINT ...]
                        pring args
```
