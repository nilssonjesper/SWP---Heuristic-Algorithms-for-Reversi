import matplotlib.pyplot as plt
import numpy as np
import random as r

#initialise seed
r.seed()

choiceList = [-1, 1]
value = 0

revs = 100000

# arrays for plotting & bar graph
xpoints = np.array([0])
ypoints = np.array([0])

ybar = ['heads', 'tails']

# streak values (boolean: true is heads)
streak = False
streakh = 0
streakt= 0
cur_streak = 0

# array stores when value reaches 0
zeroval = np.array([0])

# generate values 
for i in range(1, revs):
    rand = r.choice(choiceList)
    
    #streak
    if(streak and rand == -1):
        streakh = max(streakh, cur_streak)
        cur_streak = 0
        streak = not streak
    elif (not streak and rand == 1):
        streakt = max(streakt, cur_streak)
        cur_streak = 0
        streak = not streak
    cur_streak+=1

    if (value == 0):
        zeroval = np.append(zeroval, i)
    value += rand
    xpoints = np.append(xpoints, i)
    ypoints = np.append(ypoints, value)

# heads = 1; tails = -1
if (value >= 0):
    heads = value + revs/2
    tails = revs - heads
else:
    tails = revs/2 - value
    heads = revs - tails

# array for bar graph
headtail = [heads, tails]

# max values
maxval = np.max(ypoints)
minval = np.min(ypoints)

print("revs: ", revs)
print("heads: ", heads)
print("tails: ", tails)
print("max value", maxval)
print("min value", minval)
print("max heads streak", streakh)
print("max tails streak", streakt)
print("final value", value)
print("coin throws with value 0", zeroval)

# creating graphs
plt.figure(1)
plt.plot(xpoints, ypoints, '.', markeredgewidth=2)
plt.ylabel("Value")
plt.xlabel("Number of coin throws")
plt.figure(2)
plt.bar(ybar, headtail, label=['Heads', 'Tails'], color=['#088F8F', '#AA4A44'])
plt.ylabel("Amount")

plt.show()

input()