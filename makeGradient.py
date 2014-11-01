import colorsys
import math

def grad(a, b, steps):
    r = [list((0,0,0)) for _ in xrange(steps)]
    for i in xrange(3):
        d = float(b[i]-a[i])/(steps-1)
        for j in xrange(steps):
            r[j][i] = a[i]+d*j
    return r

def twoColors():
    a = (255, 255, 0)
    b = (255, 0, 0)
    steps = 16
    return grad(a, b, steps)


def rainbow():
    a = (1.0, 1.0, 1.0)
    b = (0.0, 1.0, 1.0)
    steps = 32 #64
    h = grad(a, b, steps)
    m = [colorsys.hsv_to_rgb(*x) for x in h]
    r = [(r*255, g*255, b*255) for (r,g,b) in m]
    return r

def sinus():
    steps = 64
    s = [math.sin(float(x)/steps * 2.0*math.pi) for x in xrange(0,steps)]
    bs = [int((x + 1.0)*128) for x in s]
    print bs

def main():
    print "---"
    r = rainbow()
    #r = twoColors()
    for c in r:
        print "0%02x%02x%02xh," % tuple(c),
    print     

main()
#sinus()    