import sys

fn = sys.argv[1] if len(sys.argv)>1 else 'D:\Work\Truba\demo\sample_v1_sketch_file.skt'

string = open(fn, 'rb').read()


decoded = string.decode('zip')
print decoded



