/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main/php.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_objects.h"
#include "Zend/zend_operators.h"

#include "internal_quantity.h"

static zend_class_entry *yumemi_internal_quantity_class_entry;
static zend_object_handlers yumemi_internal_quantity_handlers;

static zend_object *yumemi_internal_quantity_create_object(zend_class_entry *class_entry)
{
    zend_object *object = zend_objects_new(class_entry);

    object_properties_init(object, class_entry);
    object->handlers = &yumemi_internal_quantity_handlers;

    return object;
}

static zend_object *yumemi_internal_quantity_clone_object(zend_object *old_object)
{
    zend_object *new_object = yumemi_internal_quantity_create_object(old_object->ce);

    zend_objects_clone_members(new_object, old_object);

    return new_object;
}

static zend_result yumemi_internal_quantity_do_operation(zend_uchar opcode, zval *result, zval *left, zval *right)
{
    const char *method_name;
    size_t method_name_length;
    zend_function *method;
    zval *receiver;
    zval *argument;
    zval return_value;
    bool receiver_is_right = false;

    if (right == NULL) {
        return FAILURE;
    }

    /* PHP may swap ZEND_MUL operands before calling object handlers. */
    if (Z_TYPE_P(left) == IS_OBJECT && instanceof_function(Z_OBJCE_P(left), yumemi_internal_quantity_class_entry)) {
        receiver = left;
        argument = right;
    } else if ((opcode == ZEND_MUL || opcode == ZEND_DIV) && Z_TYPE_P(right) == IS_OBJECT &&
               instanceof_function(Z_OBJCE_P(right), yumemi_internal_quantity_class_entry)) {
        receiver = right;
        argument = left;
        receiver_is_right = true;
    } else {
        return FAILURE;
    }

    switch (opcode) {
        case ZEND_ADD:
            method_name = "add";
            method_name_length = sizeof("add") - 1;
            break;
        case ZEND_SUB:
            method_name = "sub";
            method_name_length = sizeof("sub") - 1;
            break;
        case ZEND_MUL:
            method_name = "mul";
            method_name_length = sizeof("mul") - 1;
            break;
        case ZEND_DIV:
            if (receiver_is_right) {
                method_name = "rdiv";
                method_name_length = sizeof("rdiv") - 1;
            } else {
                method_name = "div";
                method_name_length = sizeof("div") - 1;
            }
            break;
        case ZEND_POW:
            method_name = "pow";
            method_name_length = sizeof("pow") - 1;
            break;
        default:
            return FAILURE;
    }

    method = zend_hash_str_find_ptr_lc(&Z_OBJCE_P(receiver)->function_table, method_name, method_name_length);

    if (method == NULL || !(method->common.fn_flags & ZEND_ACC_PUBLIC) || method->common.fn_flags & ZEND_ACC_STATIC) {
        zend_throw_error(NULL, "Call to undefined method %s::%s()", ZSTR_VAL(Z_OBJCE_P(receiver)->name), method_name);
        return FAILURE;
    }

    ZVAL_UNDEF(&return_value);
    zend_call_known_instance_method_with_1_params(method, Z_OBJ_P(receiver), &return_value, argument);

    if (UNEXPECTED(EG(exception))) {
        if (!Z_ISUNDEF(return_value)) {
            zval_ptr_dtor(&return_value);
        }
        return FAILURE;
    }

    if (result == left) {
        zval_ptr_dtor(left);
    }
    ZVAL_COPY_VALUE(result, &return_value);

    return SUCCESS;
}

zend_result yumemi_register_internal_quantity(void)
{
    zend_class_entry class_entry;

    memcpy(&yumemi_internal_quantity_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    yumemi_internal_quantity_handlers.clone_obj = yumemi_internal_quantity_clone_object;
    yumemi_internal_quantity_handlers.do_operation = yumemi_internal_quantity_do_operation;

    INIT_NS_CLASS_ENTRY(class_entry, "jbboehr\\Yumemi", "InternalQuantity", NULL);
    yumemi_internal_quantity_class_entry = zend_register_internal_class(&class_entry);
    yumemi_internal_quantity_class_entry->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
    yumemi_internal_quantity_class_entry->create_object = yumemi_internal_quantity_create_object;

    return SUCCESS;
}
