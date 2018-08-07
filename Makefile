.PHONY:clean

ping:ping.c
	cc $^ -o $@

clean:
	rm ping

