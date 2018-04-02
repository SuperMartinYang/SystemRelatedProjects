print("| U0 | U1 | U2 | Element |")
print("|--|--|--|--------|")
stack = []
for a in (0, 1, 2):
    for b in (0, 1, 2):
            for c in (0, 1, 2):
                    stack.append(str(a) if a != 0 else '')
                    stack.append(((str(b) if b > 1 else '') + 'x') if b != 0 else '')
                    stack.append(((str(c) if c > 1 else '') + 'x^2') if c != 0 else '')
                    while stack and stack[-1] == '':
                            stack.pop()
                    element = stack.pop() if stack else ''
                    while stack:
                            element = stack[-1] + ' + ' + element if stack[-1] != '' else element
                            stack.pop()
                    print("|" + str(a) + "|" + str(b) + "|" + str(c) + "|" + (element if element != '' else '0') + "|")

# Galios field multiplicative inverse

# SPN
w = int()
def permute(w, P):
    new = 0
    for i in range(len(P)):
            new |= ((w & (1 << (15 - i))) >> (15 - i)) << (16 - P[i])
    return new
permute()