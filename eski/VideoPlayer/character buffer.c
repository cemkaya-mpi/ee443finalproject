int main(void){
	
	
	int offset,x =40 ,y =30;
	char *text_ptr= "asdf"; //notice that the data size is not dynamic, it is constant. It can be adjusted as in example code below.
	
	int temp=0;
	/* Display a null-terminated text string at coordinates x, y. Assume that the text fits on one line */
	offset = (y <<7) + x;
	while ( temp<6 ){
		*(unsigned char *)(0xC9000000 + offset) = text_ptr[temp]; // write to the character buffer
		temp++;
		offset++;
	}
	
	
	/* example code
	int offset;
	char *text_ptr;
	
	//Display a null-terminated text string at coordinates x, y. Assume that the text fits on one line 
	offset = (y <<7) + x;
	while ( *(text_ptr) )
	{
	*(0xC9000000 + offset) = *(text_ptr); // write to the character buffer
	++text_ptr;
	++offset;
	}
	*/
 return 0;
}


