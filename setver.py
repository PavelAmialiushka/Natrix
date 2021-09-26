#!python3
#encoding: utf8

import os, sys, datetime

#
# HEADER
#

# select version number
version = '2.9.64'
version = version.split('.')

# select release stage
stage = ['stable', 'beta'][0]

#
#  END OF HEADER
#

def install_version():
    d = datetime.date.today()
    d = '{:02}_{:02}_{:02}'.format(d.day, d.month, d.year)
    return '_'.join(version + [stage,d])

def save_to_files():
    files = [ r'src\ui\version.h', r'src\ui\mainwindow.rc' ]

    vs = {  '{VERSION_}':'_'.join(version) ,
            '{VERSION.}':'.'.join(version) ,
            '{VERSION,}':','.join(version)
        }

    for filename in files:
        f = open(filename, 'r', encoding='utf8')
    
        def scanFile(f):
            skipNext = 0
            for line in f:
    
                # поиск сигнатур
                for template in vs:
                    if template in line:
                        yield line
    
                        line = line.replace(template, vs[template]).strip()[2:] + '\n'
                        yield line
                        
                        skipNext = 2
                        # допускается не более одной сигнатуры в строке
                        break 
                
                if skipNext:
                    skipNext -= 1
                else:
                    yield line
    
    
        o = open(filename + '.out', 'w+', encoding='utf8')
        for line in scanFile(f):
            o.write( line )
        o.close()
        f.close()
    
        os.rename(filename, filename + '.tmp')
        os.rename(filename+'.out', filename)
        os.unlink(filename + '.tmp')
        print(f'modified: {filename}.')

def replace_version(doChange):
    text = ''
    for line in open('setver.py', 'rt', encoding='utf8'):
        if line.startswith("version = '"):
            line = line.split("'")
            vx = list(map(int, line[1].split('.')))
            
            vx = doChange(vx)
            global version
            version = list(map(str, vx))

            line[1] = ".".join(version)
            line = "'".join(line)

        text += line
    open('setver.py', 'wt+', encoding='utf8').write(text)
    save_to_files()


if len(sys.argv) > 1:
    if sys.argv[1] in ['++', '/a']:
        def increase(vx):
            vx[-1] += 1
            return vx
        replace_version(increase)
    elif sys.argv[1] == '-show':
        print(install_version())
        sys.exit()
    else:
        nvx = sys.argv[1].split('.')
        if len(nvx)>1:
            def set_version(vx):
                l = len(nvx)
                vx[0:l] = nvx
                return vx

            replace_version(set_version)

if __name__=='__main__':
    print('.'.join(version))
