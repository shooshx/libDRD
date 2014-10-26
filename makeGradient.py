
a = (255, 255, 0)
b = (255, 0, 0)
steps = 30

r = [list((0,0,0)) for _ in xrange(steps)]
for i in xrange(3):
    d = float(b[i]-a[i])/(steps-1)
    for j in xrange(steps):
        r[j][i] = a[i]+d*j

print "---"
for c in r:
    print "0%02x%02x%02xh," % tuple(c),
print     
