{
	int X

	Function inc {int A} {} {
		return A# + 1;
	}
Begin
	X = 2;
	inc[X#];
End
}
