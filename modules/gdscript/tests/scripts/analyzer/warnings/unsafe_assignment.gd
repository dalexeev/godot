func test():
	var _hard_int: int = 1
	var _variant_int: Variant = 1
	var _weak_int = 1

	_hard_int = _hard_int
	_hard_int = _variant_int
	_hard_int = _weak_int

	_variant_int = _hard_int
	_variant_int = _variant_int
	_variant_int = _weak_int

	_weak_int = _hard_int
	_weak_int = _variant_int
	_weak_int = _weak_int

	_hard_int += _hard_int
	_hard_int += _variant_int
	_hard_int += _weak_int

	_variant_int += _hard_int
	_variant_int += _variant_int
	_variant_int += _weak_int

	_weak_int += _hard_int
	_weak_int += _variant_int
	_weak_int += _weak_int
