import random
import sys

def rand_char():
	return unichr(random.randint(ord('A'),ord('z')))
	#return unichr(random.randint(ord('A'),ord('Z')))

def gen_pattern(len):
	pattern = ""
	for i in range(len):
		pattern += rand_char()
	return pattern
	
# for pattern_prob just give the denominator of a probability of pattern appearing. eg for 
# probability 1/n, give n.
def gen_text(pattern, text_len, pattern_prob):
	text = ""
	positions = []
	j = 0
	for i in range(0, text_len):
		if random.randint(1,pattern_prob) == 1 and i < text_len - len(pattern)+1:
			text += pattern
			positions.append(j)
			#i+=len(pattern)
			j+=len(pattern)-1

		else:
			text += rand_char()
		j+=1
	return text,positions

pattern_len = int(sys.argv[1])
text_len = int(sys.argv[2])
pattern_prob = int(sys.argv[3])

num_tests = int(sys.argv[4])

cf = open("control.txt", "w")
rf = open("results.txt", "w")
for i in range(1,num_tests+1):
	pattern = gen_pattern(pattern_len)

	pf = open("pattern"+str(i)+".txt", "w")
	pf.write(pattern)

	result = gen_text(pattern, text_len, pattern_prob)

	tf = open("text"+str(i)+".txt", "w")
	tf.write(str(result[0]))

	if len(result[1]) == 0:
		s = str(i) + " " + str(i) + " -1\n"
		rf.write(s)

	else:
		s = str(i) + " " + str(i) + " " + str(min(result[1])) + "\n"
		rf.write(s)

	cf.write("0 " + str(i) + " " + str(i)+"\n")

	
cf.close()
rf.close()
