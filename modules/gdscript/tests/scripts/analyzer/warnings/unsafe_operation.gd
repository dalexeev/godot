func test_no_exec():
	var hard_int: int = 1
	var variant_int: Variant = 1
	var weak_int = 1

	print(hard_int + hard_int)
	print(hard_int + variant_int)
	print(hard_int + weak_int)
	print(variant_int + variant_int)
	print(variant_int + weak_int)
	print(weak_int + weak_int)

	print(-hard_int)
	print(-variant_int)
	print(-weak_int)


func test():
	pass
