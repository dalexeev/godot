var variant_int_var: Variant = 1


func my_func(_hard_int_param: int = variant_int_var) -> void:
	pass


func test():
	var hard_int: int = 1
	var variant_int: Variant = 1
	var weak_int = 1

	var _hard_int_1: int = hard_int
	var _hard_int_2: int = variant_int
	var _hard_int_3: int = weak_int

	var _variant_int_1: Variant = hard_int
	var _variant_int_2: Variant = variant_int
	var _variant_int_3: Variant = weak_int

	var _weak_int_1 = hard_int
	var _weak_int_2 = variant_int
	var _weak_int_3 = weak_int
