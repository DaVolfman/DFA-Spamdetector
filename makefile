default: myhttpd.c
	g++ -pthread -o spamdetector spamdetector.cpp
clean:
	$(RM) spamdetector

