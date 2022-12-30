import math
import numpy

vbits = 256

f_r5 = numpy.array([
    1500000, 1285828, 1928409, 1286978, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
])
f_gan = numpy.array([
    278398, 137234, 377986, 25582, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
])


def xor_align(x, y):
    return min(max(x, y), 64)

def mul_align(x, y):
    return min(x + y, 64)

def prob(f, g, align_func, align):
    pf = f / numpy.sum(f)
    pg = g / numpy.sum(g)

    p = {}
    for i in range(len(f)):
        if (f[i] == 0):
            continue
        for j in range(len(g)):
            if (g[j] == 0):
                continue
            w = align_func(i, j)
            w = int(math.ceil(w / align)) * align

            if w not in p:
                p[w] = 0.0
            p[w] += pf[i] * pg[j]

    return p


def analytical(f, align_func, align):
    ans = {}

    p = prob(f, f, align_func, align)
    u = {0:1.0}
    v = {}
    e = 0

    while (len(u) != 0):
        for k in u:
            for w in p:
                l = k + w
                if l not in v:
                    v[l] = 0
                v[l] += u[k] * p[w]

        u = {}
        s = 0
        for k in v:
            if (k > vbits):
                if e not in ans:
                    ans[e] = 0
                ans[e] += v[k]
            else:
                u[k] = v[k]
                s += v[k]
        v = {}
        e += 1
        if s < 1e-8:
            break

    return ans

def run(name, f, align_func, align):
    name += ("_a" + str(align))
    ans = analytical(f, align_func, align)
    s = sum(ans.values())
    m = 0
    for k in ans:
        m += k * ans[k] / s

    print("%s: s=%.12lf m=%.4lf" % (name, s, m))
    return m

r5m = [
    run("RAID5", f_r5, xor_align, 8),
    run("RAID5", f_r5, xor_align, 4),
    run("RAID5", f_r5, xor_align, 2),
    run("RAID5", f_r5, xor_align, 1),
]
gam = [
    run("GAN", f_gan, mul_align, 8),
    run("GAN", f_gan, mul_align, 4),
    run("GAN", f_gan, mul_align, 2),
    run("GAN", f_gan, mul_align, 1),
]

print("\n")
for x in r5m:
    print(x, end="\t")
print()
for x in gam:
    print(x, end="\t")
print()
