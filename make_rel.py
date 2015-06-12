import sys
import os

def run(s):
    print "**", s
    os.system(s)
    
def main(argv):
    if len(argv) < 2:
        print "Usage: make_rel.py REL_NUM [Debug|Release]"
        return 1
    num = sys.argv[1]
    dir = os.path.abspath(os.path.dirname(__file__))
    cfg = "Debug"
    if len(argv) >= 3:
        cfg = argv[2]

    reldir = dir + r"\rel\rel_" + num + "_" + cfg;
    if not os.path.exists(reldir):
        os.mkdir(reldir)
        
    run(r"copy %s\bin\%s\*.exe %s" % (dir, cfg, reldir))
    run(r"copy %s\rainbow_dash.bmp %s" % (dir, reldir))
    run(r"copy %s\*.wav %s" % (dir, reldir))
    run(r"copy %s\cpp_turtle_readme.txt %s" % (dir, reldir))
    
    run(r"copy %s\bin\%s\drd.lib %s" % (dir, cfg, reldir))
    run(r"copy %s\drd.inc %s" % (dir, reldir))
    run(r"copy %s\drd.h %s" % (dir, reldir))
    
    run(r"copy c:\windows\SysWOW64\msvcr120.dll " + reldir)
    run(r"copy c:\windows\SysWOW64\msvcp120.dll " + reldir)

if __name__ == "__main__":
    sys.exit(main(sys.argv))