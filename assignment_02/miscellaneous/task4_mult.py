import matplotlib.pyplot as plt
import numpy as np
import random as r

#initialise seed
r.seed()

choiceList = [-1, 1]
value = 0

revs = 10000

# arrays for plotting & bar graph
xpoints = np.array([0])
ypoints0 = np.array([0])
ypoints1 = np.array([0])
ypoints2 = np.array([0])
ypoints3 = np.array([0])
ypoints4 = np.array([0])

# generate values 
for i in range(1, revs):
    rand = r.choice(choiceList)

    value += rand
    xpoints = np.append(xpoints, i)
    ypoints0 = np.append(ypoints0, value)
value = 0

for i in range(1, revs):
    rand = r.choice(choiceList)

    value += rand
    ypoints1 = np.append(ypoints1, value)
value = 0

for i in range(1, revs):
    rand = r.choice(choiceList)

    value += rand
    ypoints2 = np.append(ypoints2, value)
value = 0

for i in range(1, revs):
    rand = r.choice(choiceList)

    value += rand
    ypoints3 = np.append(ypoints3, value)
value = 0

for i in range(1, revs):
    rand = r.choice(choiceList)

    value += rand
    ypoints4 = np.append(ypoints4, value)

print("revs: ", revs)

# creating graphs
plt.plot(xpoints, ypoints0)
plt.plot(xpoints, ypoints1)
plt.plot(xpoints, ypoints2)
plt.plot(xpoints, ypoints3)
plt.plot(xpoints, ypoints4)
plt.ylabel("Value")
plt.xlabel("Number of coin throws")

plt.show()