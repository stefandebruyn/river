# Format with clang-format.
format:
	find . -regex '.*\.\(cpp\|hpp\)' -exec clang-format -i -style=file {} \;

# Build and run unit tests from scratch.
.PHONY: test
test:
	-rm -rf build
	mkdir build && cd build && cmake .. && make test && ./test -v
