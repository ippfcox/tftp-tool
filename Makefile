all: s c

c:
	@make -Ctftpc all

s:
	@make -Ctftps all

clean:
	@make -Ctftpc clean
	@make -Ctftps clean