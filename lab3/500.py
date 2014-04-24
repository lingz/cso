with open("500.txt", "w") as f:
  for i in xrange(500):
    f.write("90 ")
    if i%10 == 0:
      f.write("\n")
