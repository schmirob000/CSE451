#include <inc/assert.h>
#include <inc/malloc.h>
#include <inc/string.h>

#define GO_PTR			22
#define GO_UNSAFE_POINTER	26

#define GO_DIRECT_IFACE		(1 << 5)

#define GO_CODE_MASK		0x1f

struct go_string {
	const void *data;
	int length;
};

struct go_function {
	void (*fn)(void);
};

struct go_type_descriptor {
	unsigned char code;
	unsigned char align;
	unsigned char field_align;
	uintptr_t size;
	uint32_t hash;
	const struct go_function *hashfn;
	const struct go_function *equalfn;
	const void *gc;
	const struct go_string *reflection;
	const void *uncommon;
	const struct go_type_descriptor *pointer_to_this;
};

struct go_empty_interface {
	const struct go_type_descriptor *type_descriptor;
	void *object;
};

struct go_open_array {
	void *values;
	int count;
	int capacity;
};

// entry point
extern void gomain(void) asm("main.main");

void umain(int argc, char **argv)
{
	gomain();
}

// runtime

extern const struct go_type_descriptor *
efacetype(struct go_empty_interface e)
asm("runtime.efacetype");

extern _Bool
__go_type_descriptors_equal(const struct go_type_descriptor *td1,
			    const struct go_type_descriptor *td2)
asm("runtime.ifacetypeeq");

uintptr_t
__go_type_hash_identity(const void *key, uintptr_t key_size)
{
	uintptr_t i, hash = 5381;
	const uint8_t *p = key;

	for (i = 0; i < key_size; ++i)
		hash = hash * 33 + p[i];
	return hash;
}

_Bool
__go_type_equal_identity(const void *k1, const void *k2, uintptr_t key_size)
{
	return memcmp(k1, k2, key_size) == 0;
}

void
__go_type_hash_error(const void *val, uintptr_t key_size)
{
	panic("go type hash error");
}

void
__go_type_equal_error(const void *v1, const void *v2, uintptr_t key_size)
{
	panic("go type equal error");
}

uintptr_t
__go_type_hash_string(const void *key, uintptr_t key_size)
{
	const struct go_string *s = key;

	return __go_type_hash_identity(key, s->length);
}

static inline _Bool
__go_is_pointer_type(const struct go_type_descriptor *td)
{
	unsigned char code = td->code & GO_CODE_MASK;
	return (code == GO_PTR) || (code == GO_UNSAFE_POINTER);
}

static inline uintptr_t
__go_call_hashfn(const struct go_function *hashfn, const void *p, uintptr_t size)
{
	uintptr_t (*h)(const void *, uintptr_t) = (void *)hashfn->fn;
	return __builtin_call_with_static_chain(h(p, size), hashfn);
}

static inline uintptr_t
__go_call_equalfn(const struct go_function *equalfn, const void *p1, const void *p2, uintptr_t size)
{
	uintptr_t (*e)(const void *, const void *, uintptr_t) = (void *)equalfn->fn;
	return __builtin_call_with_static_chain(e(p1, p2, size), equalfn);
}

uintptr_t
__go_type_hash_empty_interface(const struct go_empty_interface *val, uintptr_t key_size)
{
	const struct go_type_descriptor *td = val->type_descriptor;
	const void *p;

	if (!td)
		return 0;
	if (__go_is_pointer_type(td))
		p = &val->object;
	else
		p = val->object;
	return __go_call_hashfn(td->hashfn, p, td->size);
}

static _Bool
__go_strings_equal(struct go_string s1, struct go_string s2)
{
	return s1.length == s2.length && memcmp(s1.data, s2.data, s1.length) == 0;
}

static _Bool
__go_ptr_strings_equal(const struct go_string *ps1, const struct go_string *ps2)
{
	if (ps1 == ps2)
		return 1;
	if (!ps1 || !ps2)
		return 0;
	return __go_strings_equal(*ps1, *ps2);
}

_Bool
__go_type_equal_string(const void *k1, const void *k2, uintptr_t key_size)
{
	return __go_ptr_strings_equal(k1, k2);
}

int
__go_empty_interface_compare(struct go_empty_interface v1,
			     struct go_empty_interface v2)
{
	const struct go_type_descriptor *td1, *td2;

	td1 = v1.type_descriptor;
	td2 = v2.type_descriptor;

	if (td1 == td2)
		return 0;
	if (!td1 || !td2)
		return 1;
	if (!__go_type_descriptors_equal(td1, td2))
		return 1;
	if (__go_is_pointer_type(td1))
		return v1.object == v2.object ? 0 : 1;
	if (!__go_call_equalfn(td1->equalfn, v1.object, v2.object, td1->size))
		return 1;
	return 0;
}

_Bool
__go_type_equal_empty_interface(const struct go_empty_interface *v1,
				const struct go_empty_interface *v2,
				uintptr_t key_size)
{
	return __go_empty_interface_compare(*v1, *v2) == 0;
}

const struct go_function __go_type_hash_identity_descriptor = {
	(void *)__go_type_hash_identity
};

const struct go_function __go_type_equal_identity_descriptor = {
	(void *)__go_type_equal_identity
};

const struct go_function __go_type_hash_error_descriptor = {
	(void *)__go_type_hash_error
};

const struct go_function __go_type_equal_error_descriptor = {
	(void *)__go_type_equal_error
};

const struct go_function __go_type_hash_empty_interface_descriptor = {
	(void *)__go_type_hash_empty_interface
};

const struct go_function __go_type_hash_string_descriptor = {
	(void *)__go_type_hash_string
};

const struct go_function __go_type_equal_string_descriptor = {
	(void *)__go_type_equal_string
};

const struct go_function __go_type_equal_empty_interface_descriptor = {
	(void *)__go_type_equal_empty_interface
};

void
__go_runtime_error(int32_t i)
{
	panic("go runtime error: %d", i);
}

int
__go_strcmp(struct go_string s1, struct go_string s2)
{
	int r;

	r = memcmp(s1.data, s2.data, MIN(s1.length, s2.length));
	if (r)
		return r;
	if (s1.length < s2.length)
		return -1;
	if (s1.length > s2.length)
		return 1;
	return 0;
}

void *
__go_new(const struct go_type_descriptor *td, uintptr_t size)
{
	return malloc(size);
}

// compatibility: for gcc 5
void *
__go_new_nopointers(const struct go_type_descriptor *td, uintptr_t size)
{
	return __go_new(td, size);
}

// unsafe pointer

struct field_align {
	char c;
	void *p;
};

static char unsafe_pointer_reflection_str[] = "unsafe.Pointer";

static const struct go_string unsafe_pointer_reflection = {
	unsafe_pointer_reflection_str,
	sizeof(unsafe_pointer_reflection_str) - 1,
};

const struct go_type_descriptor unsafe_Pointer asm("__go_tdn_unsafe.Pointer") = {
	.code		= GO_UNSAFE_POINTER | GO_DIRECT_IFACE,
	.align		= __alignof(void *),
	.field_align	= offsetof (struct field_align, p) - 1,
	.size		= sizeof(void *),
	.hash		= 78501163U,
	.hashfn		= &__go_type_hash_identity_descriptor,
	.equalfn	= &__go_type_equal_identity_descriptor,
	.reflection	= &unsafe_pointer_reflection,
};

// string

struct go_string
__go_byte_array_to_string(const void *data, int length)
{
	struct go_string s;
	void *p;

	p = malloc(length);
	memcpy(p, data, length);
	s.data = p;
	s.length = length;
	return s;
}

struct go_string
__go_string_plus(struct go_string s1, struct go_string s2)
{
	struct go_string s;
	void *p;

	s.length = s1.length + s2.length;
	p = malloc(s.length);
	assert(p);
	memcpy(p, s1.data, s1.length);
	memcpy(p + s1.length, s2.data, s2.length);
	s.data = p;
	return s;
}

// interface

const struct go_type_descriptor *
efacetype(struct go_empty_interface e) {
	return e.type_descriptor;
}

_Bool
__go_type_descriptors_equal(const struct go_type_descriptor *td1,
			    const struct go_type_descriptor *td2)
{
	if (td1 == td2)
		return 1;
	if (!td1 || !td2)
		return 0;
	if (td1->code != td2->code || td1->hash != td2->hash)
		return 0;
	return __go_ptr_strings_equal(td1->reflection, td2->reflection);
}

void
__go_check_interface_type(const struct go_type_descriptor *lhs_descriptor,
			  const struct go_type_descriptor *rhs_descriptor,
			  const struct go_type_descriptor *rhs_inter_descriptor)
{
}

// array

struct go_open_array
__go_append(struct go_open_array a, void *bvalues, uintptr_t bcount, uintptr_t element_size)
{
	int count;

	if (!bvalues || bcount == 0)
		return a;
	assert(bcount <= INT_MAX - a.count);
	count = a.count + bcount;
	if (count > a.capacity) {
		void *p = malloc(count * element_size);

		assert(p);
		memcpy(p, a.values, a.count * element_size);
		a.values = p;
		a.capacity = count;
	}
	memmove(a.values + a.count * element_size, bvalues, bcount * element_size);
	a.count = count;
	return a;
}

