import sys
import os, glob

def convert(filename):
    if ".c" not in filename:
        return
    #filename = sys.argv[1]
    f = open(filename, 'r')
    lines = f.readlines()
    for line in lines:
        if 'strerror' in line:
            print line

    newname = filename.split('.')[0]+'%d.c' % 1 
    f1 = open(newname, 'w+')
    print newname
    for line in lines:
        if 'strerror' in line and 'strerror_r' not in line:
            count = 0 
            for a in line:
                if a == ' ':
                    count += 1
                elif a == '\t':
                    count += 4
                else:
                    break
            #for i in range(0, count):
            #    f1.write(' ')
            f1.write(' '*count + 'char errbuf[1024];\n')
            #for i in range(0, count):
            #    f1.write(' ')
            f1.write(' '*count +'strerror_r(errno, errbuf, 1024);\n')
            newline = line.split('strerror')
            f1.write(newline[0] + ' errbuf);\n')
            continue
        f1.write(line.replace('\t', '    '))
    
    f.close()
    f1.close()
    os.rename(newname, filename)

if __name__ == '__main__':
    if os.path.isdir(sys.argv[1]):
        for f in os.listdir(sys.argv[1]):
            print 'file:', f
            convert(f)
    else:
        convert(sys.argv[1])



