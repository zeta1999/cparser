/*
 * This file is part of cparser.
 * Copyright (C) 2012 Matthias Braun <matze@braunis.de>
 */
#ifndef ENTITY_T_H
#define ENTITY_T_H

#include <libfirm/firm_types.h>

#include "adt/util.h"
#include "ast/ast.h"
#include "ast/attribute.h"
#include "ast/entity.h"
#include "ast/symbol.h"
#include "firm/jump_target.h"
#include "position.h"

typedef enum {
	ENTITY_VARIABLE = 1,
	ENTITY_COMPOUND_MEMBER,
	ENTITY_PARAMETER,
	ENTITY_FUNCTION,
	ENTITY_TYPEDEF,
	ENTITY_CLASS,
	ENTITY_STRUCT,
	ENTITY_UNION,
	ENTITY_ENUM,
	ENTITY_ENUM_VALUE,
	ENTITY_LABEL,
	ENTITY_LOCAL_LABEL,
	ENTITY_NAMESPACE,
	ENTITY_ASM_ARGUMENT,
} entity_kind_t;

typedef enum entity_namespace_t {
	NAMESPACE_NORMAL = 1,   /**< entity is a normal declaration */
	NAMESPACE_TAG,          /**< entity is a struct/union/enum tag */
	NAMESPACE_LABEL,        /**< entity is a (goto) label */
	NAMESPACE_ASM_ARGUMENT, /**< entity is an asm statement argument */
} entity_namespace_t;

typedef enum storage_class_t {
	STORAGE_CLASS_NONE,
	STORAGE_CLASS_EXTERN,
	STORAGE_CLASS_STATIC,
	STORAGE_CLASS_TYPEDEF,
	STORAGE_CLASS_AUTO,
	STORAGE_CLASS_REGISTER,
} storage_class_t;

typedef enum elf_visibility_t {
	ELF_VISIBILITY_DEFAULT,
	ELF_VISIBILITY_HIDDEN,
	ELF_VISIBILITY_INTERNAL,
	ELF_VISIBILITY_PROTECTED,
	ELF_VISIBILITY_ERROR
} elf_visibility_t;

typedef enum {
	BUILTIN_NONE = 0,
	BUILTIN_ALLOCA,
	BUILTIN_CIMAG,
	BUILTIN_CREAL,
	BUILTIN_EXPECT,
	BUILTIN_FIRM,
	BUILTIN_INF,
	BUILTIN_LIBC,
	BUILTIN_LIBC_CHECK,
	BUILTIN_NAN,
	BUILTIN_OBJECT_SIZE,
	BUILTIN_ROTL,
	BUILTIN_ROTR,
	BUILTIN_VA_END,
} builtin_kind_t;

/**
 * A scope containing entities.
 */
struct scope_t {
	entity_t *entities;
	entity_t *last_entity; /**< pointer to last entity (so appending is fast) */
	unsigned  depth;       /**< while parsing, the depth of this scope in the
	                            scope stack. */
};

/**
 * a named entity is something which can be referenced by its name
 * (a symbol)
 */
struct entity_base_t {
	ENUMBF(entity_kind_t)      kind    : 8;
	ENUMBF(entity_namespace_t) namespc : 8;
	symbol_t                  *symbol;
	position_t                 pos;
	/** The scope where this entity is contained in */
	scope_t                   *parent_scope;
	entity_t                  *parent_entity;
	/** next declaration in a scope */
	entity_t                  *next;
	/** next declaration with same symbol */
	entity_t                  *symbol_next;
};

struct compound_t {
	entity_base_t     base;
	entity_t         *alias; /* used for name mangling of anonymous types */
	scope_t           members;
	decl_modifiers_t  modifiers;
	attribute_t      *attributes;
	bool              complete          : 1;
	bool              transparent_union : 1;
	bool              packed            : 1;

	il_alignment_t    alignment;
	il_size_t         size;
};

struct enum_t {
	entity_base_t  base;
	entity_t      *alias; /* used for name mangling of anonymous types */
	bool           complete : 1;
};

struct enum_value_t {
	entity_base_t  base;
	expression_t  *value;
	type_t        *enum_type;

	/* ast2firm info */
	ir_tarval     *tv;
};

struct label_t {
	entity_base_t  base;
	bool           used          : 1;
	bool           address_taken : 1;
	/* Reference counter to mature the label block as early as possible. */
	unsigned       n_users;
	statement_t   *statement;

	/* ast2firm info */
	jump_target    target;
	ir_node       *indirect_block;
};

struct namespace_t {
	entity_base_t base;
	scope_t       members;
};

struct typedef_t {
	entity_base_t     base;
	decl_modifiers_t  modifiers;
	type_t           *type;
	il_alignment_t    alignment;
	bool              builtin : 1;
};

struct declaration_t {
	entity_base_t           base;
	type_t                 *type;
	ENUMBF(storage_class_t) declared_storage_class : 8;
	ENUMBF(storage_class_t) storage_class          : 8;
	/** Set if the declaration is used. */
	bool                    used                   : 1;
	/** Set for implicit (not found in source code) declarations. */
	bool                    implicit               : 1;
	decl_modifiers_t        modifiers;
	il_alignment_t          alignment;
	attribute_t            *attributes;

	/* ast2firm info */
	unsigned char     kind;
};

struct compound_member_t {
	declaration_t  base;
	il_size_t      offset;     /**< the offset of this member in the compound */
	unsigned char  bit_offset; /**< extra bit offset for bitfield members */
	unsigned char  bit_size;   /**< bitsize for bitfield members */
	bool           bitfield : 1;  /**< member is (part of) a bitfield */

	/* ast2firm info */
	ir_entity *entity;
};

struct variable_t {
	declaration_t            base;
	bool                     thread_local   : 1;
	/** Set if the address of this declaration was taken. */
	bool                     address_taken  : 1;
	bool                     read           : 1;
	ENUMBF(elf_visibility_t) elf_visibility : 3;

	initializer_t *initializer;
	union {
		symbol_t  *symbol;
		entity_t  *entity;
	} alias;                            /**< value from attribute((alias())) */

	/* ast2firm info */
	union {
		unsigned int  value_number;
		ir_entity    *entity;
		ir_node      *vla_base;
	} v;
};

struct function_t {
	declaration_t  base;
	/** builtin kind */
	ENUMBF(builtin_kind_t)   btk            : 8;
	bool                     is_inline      : 1;
	/** Inner function needs closure. */
	bool                     need_closure   : 1;
	/** Inner function has goto to outer function. */
	bool                     goto_to_outer  : 1;
	ENUMBF(elf_visibility_t) elf_visibility : 3;
	/** builtin is library, this means you can safely take its address */
	bool                     builtin_in_lib : 1;
	scope_t        parameters;
	statement_t   *body;
	symbol_t      *actual_name;        /**< gnu extension __REDIRECT */
	union {
		symbol_t  *symbol;
		entity_t  *entity;
	} alias;                           /**< value from attribute((alias())) */

	/* ast2firm info */
	union {
		ir_builtin_kind firm_builtin_kind;
		unsigned        chk_arg_pos;
	} b;
	ir_entity      *irentity;
	ir_node        *static_link;        /**< if need_closure is set, the node
	                                         representing the static link. */
};

struct asm_argument_t {
	entity_base_t base;
	string_t      constraints;
	expression_t *expression;
	unsigned      pos;
	bool          direct_read    :1;/**< argument value is read */
	bool          direct_write   :1;/**< argument is lvalue and written to */
	bool          indirect_read  :1;/**< argument is address which is read */
	bool          indirect_write :1;/**< argument is address which is written */
};

union entity_t {
	ENUMBF(entity_kind_t) kind : 8;
	entity_base_t         base;
	compound_t            compound;
	enum_t                enume;
	enum_value_t          enum_value;
	label_t               label;
	namespace_t           namespacee;
	typedef_t             typedefe;
	declaration_t         declaration;
	variable_t            variable;
	function_t            function;
	compound_member_t     compound_member;
	asm_argument_t        asm_argument;
};

#define DECLARATION_KIND_CASES \
	     ENTITY_FUNCTION:        \
	case ENTITY_VARIABLE:        \
	case ENTITY_PARAMETER:       \
	case ENTITY_COMPOUND_MEMBER

static inline bool is_declaration(const entity_t *entity)
{
	switch (entity->kind) {
	case DECLARATION_KIND_CASES:
		return true;
	default:
		return false;
	}
}

const char *get_entity_kind_name(entity_kind_t kind);

entity_t *allocate_entity_zero(entity_kind_t, entity_namespace_t, symbol_t*, position_t const*);

elf_visibility_t get_elf_visibility_from_string(const char *string);

entity_t *skip_unnamed_bitfields(entity_t*);

#endif