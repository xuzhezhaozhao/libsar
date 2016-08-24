.PHONY: all
all: test_sar

test_sar: ioconf.h ioconf.c sar.h sar.cpp SarInfo.pb.h SarInfo.pb.cc test.cpp
	g++ $^ -lprotobuf -o test_sar

SarInfo.pb.h SarInfo.pb.cc: SarInfo.proto
	protoc --cpp_out=./ $^


.PHONY: test
test: test_sar
	./test_sar

.PHONY: clean
clean:
	rm -rf SarInfo.pb.h SarInfo.pb.cc test_sar


