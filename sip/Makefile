
test: test_suite
	./test_suite

test_suite: tests/suite.c lib/sip.c lib/serialisation.c lib/request.c
	clang -std=c23 -DSIP_DEBUG=0 -fsanitize=address -o test_suite $^ -Wall -g


server: start_server
	./start_server

start_server: start_server.c lib/sip.c lib/serialisation.c lib/request.c
	clang -std=c23 -o start_server $^ -Wall -g

client: start_client
	./start_client

start_client: start_client.c lib/sip.c lib/serialisation.c
	clang -std=c23 -o start_client $^ -Wall -g
