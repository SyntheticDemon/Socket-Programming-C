all: buyer seller
buyer : buyer.c
	gcc buyer.c -o buyer
seller : seller.c
	gcc seller.c -o seller

.PHONY: clean

clean:
	rm buyer seller