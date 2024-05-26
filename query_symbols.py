import sqlite3
from sqlite3 import Cursor


def __is_array(field_id, db_cursor) -> bool:
    """
    Return True if the field with field_id is an array. Otherwise, return False.
    """
    dim_list_records = db_cursor.execute('SELECT * FROM dimension_lists WHERE field_id=?',
                                         (field_id,)).fetchall()
    return len(dim_list_records) > 0


def __is_base_type(type_name: str) -> tuple:
    """
    Checks if type_name is a base type as it appears in the database.
    :return: A tuple of the form (bool, str), where the bool is whether this is a basetype or not and what the
    base type maps to in our BaseType namespace. Please note that this function does not pre-append the BaseType
    namespace to the type, that is the responsibility of the caller. Please note that padding types are also
    considered base types. Padding types have the form of _padding[Number Of Bits] such as _padding8.

    NOTE:While strings are considered a base type, it should be noted that, as opposed to all of the other base types,
    they are created as needed. This is because we can't really predict their sizes, like we do
    ints, not even a range as a string could be of any size. Thus they are created on the fly.
    """
    out_base_type = (False, '')

    if type_name == 'int64' \
            or type_name == 'int32' \
            or type_name == 'int16' \
            or type_name == 'int8' \
            or type_name == 'int':
        out_base_type = (True, 'int')
    # FIXME: All the standardized types in https://en.cppreference.com/w/cpp/types/integer should be added.
    elif type_name == 'uint8' \
            or type_name == 'uint16' \
            or type_name == 'uint16_t' \
            or type_name == 'uint32' \
            or type_name == 'unsigned int' \
            or type_name == 'unsigned' \
            or type_name == 'uint64' \
            or type_name == 'unsigned char':
        out_base_type = (True, 'uint')
    # FIXME: char types need to be handled properly
    elif type_name == 'char':
        out_base_type = (True, 'int')
    elif type_name == 'boolean' or type_name == 'bool':
        out_base_type = (True, 'boolean')
    elif type_name == 'float' or type_name == 'double':
        out_base_type = (True, 'float')
    elif type_name[:8] == '_padding':
        out_base_type = (True, '_padding')
    elif type_name == 'string':
        out_base_type = (True, 'string')

    return out_base_type


def __is__symbol_enum(db_cursor, symbol_id: int):
    """
        Checks the database to see if the symbol id is actually an enum or not.
        :param symbol_id:
        :return: True if the symbol is an enum. Otherwise, False is returned.
        """
    return len(db_cursor.execute('SELECT * FROM enumerations where symbol=?',
                                      (symbol_id,)).fetchall()) > 0


def __get_basetype_name():
    pass


def __get_enum_byte_size(symbol_id: int, db_cursor):
    """
    Factory function that creates a EnumeratedParameterType that is described by the symbol with symbol_id
    in the database. This function assumes that the caller has ensured that there is an enumeration
    associated to the symbol_id in the database.
    :param symbol_id: id of symbol that points to this enumeration type in the database.
    :return:
    """
    symbol_elf, symbol_name, byte_size = db_cursor.execute(
        'SELECT elf, name,byte_size FROM symbols where id=?',
        (symbol_id,)).fetchone()
    # out_enum = xtce.EnumeratedParameterType(name=symbol_name)

    # little_endian = self.is_little_endian(symbol_elf)


    # for value, name in self.db_cursor.execute('SELECT value, name FROM enumerations where symbol=?',
    #                                           (symbol_id,)).fetchall():
    #     enum_list.add_Enumeration(xtce.ValueEnumerationType(label=name, value=value))

    # out_enum.set_EnumerationList(enum_list)

    return byte_size


def get_overlayed_type(symbol_record: tuple, db_cursor, data: bytes, current_data_cursor: int, out_data: dict):
    """
    A factory function to create an aggregateParamType type pointed to by symbol_id.
    :param symbol_record: A tuple containing the symbol record of the database in the form of
    (id, elf, name, byte_size)
    :return: If the symbol is processed successfully, an AggregateParameterType representing that symbol(struct) is returned.
    If the symbol already exists, then the AggregateParameterType representing this symbol is returned.
    Beware that if this function finds a field of the symbol record
    whose type does not exist(such as a field that has a struct type not defined in our xtce), then this function takes
    the liberty of adding it to the telemetry object in the xtce object.
    """

    # FIXME: This entire function needs to be decoupled; it's far too big
    # out_param = xtce.AggregateParameterType(name=symbol_record[2])

    # logging.debug(f'symbol record-->{symbol_record}')

    symbol_id = str(symbol_record[0])

    fields = db_cursor.execute('SELECT * FROM fields where symbol=?',
                               (symbol_id,)).fetchall()

    # Enforce ordering of fields by offset.
    fields.sort(key=lambda record: record[3])

    # logging.debug(f'root fields-->{fields}')

    type_ref_name = None

    # member_list = xtce.MemberListType()
    # out_param.set_MemberList(member_list)
    symbol_id = str(symbol_record[0])

    new_current_data_cursor = current_data_cursor

    symbol_name = symbol_record[2]
    out_data[symbol_name] = {}

    for field_id, field_symbol, field_name, field_byte_offset, field_type, field_little_endian, bit_size, bit_offset, short_description, long_description in fields:
        # If this field is standalone array, ignore it for now
        if field_type == field_symbol:
            continue

        elif __is_array(field_id, db_cursor):
            # logging.debug(f'comparing {field_type} and {field_symbol}')

            symbol_type = db_cursor.execute('SELECT * FROM symbols where id=?',
                                            (field_type,)).fetchone()
            # The symbol_type is expected, as per our schema, to have the form of (id, elf ,name, byte_size)
            if symbol_type:
                # logging.debug(f'symbol_type$$$$-->{symbol_type}')
                base_type_val = __is_base_type(symbol_type[2])

                if __is__symbol_enum(db_cursor, field_type) is True:
                    # FIXME
                    pass
                    # new_enum = __get_enum_param_type(field_type)
                    # self[
                    #     module_name].get_TelemetryMetaData().get_ParameterTypeSet().add_EnumeratedParameterType(
                    #     new_enum)
                    # type_ref_name = new_enum.get_name()

                elif base_type_val[0]:
                    # This is a basetype, so we can just get a type from our BaseType namespace
                    # TODO: Make a distinction between unsigned and int types
                    dim_list_records = db_cursor.execute('SELECT * FROM dimension_lists WHERE field_id=?',
                                         (field_id,)).fetchall()
                    # FIXME:Add outer loop to include as many elements as indicated by dim_list_records
                    out_data[symbol_name][field_name] = bytearray()
                    # out_data[symbol_record][field_name] += data[current_data_cursor]
                    for d in data[current_data_cursor:current_data_cursor + symbol_type[3]]:
                        new_current_data_cursor += 1
                        out_data[symbol_name][field_name].append(d)
                else:
                    # logging.debug(f'field type-->{field_type}')
                    child_symbol = db_cursor.execute('SELECT * FROM symbols where id=?',
                                                          (field_type,)).fetchone()

                    # logging.debug(f'field_symbol id:{field_symbol}')
                    # logging.debug(f'child symbol-->{child_symbol}')
                    get_overlayed_type(child_symbol, db_cursor, data, new_current_data_cursor, out_data)
                    # child = __get_aggregate_paramtype(child_symbol, module_name, False)
                    # if __aggregate_paramtype_exists(child_symbol[2], module_name) is False:
                    #     self[module_name].get_TelemetryMetaData().get_ParameterTypeSet().add_AggregateParameterType(
                    #         child)
                    # type_ref_name = child.get_name()

                # if __is__symbol_string(field_type) is True:
                #     field_multiplicity = self.__get_array(field_id)[0][3] + 1
                #     # FIXME: Don't love the way I'm handling this, looks kind of ugly and wack. Will revise.
                #     endianness_postfix = self.__get_endianness_postfix(field_little_endian)
                #     string_type_name = 'string' + str(field_multiplicity * 8) + endianness_postfix
                #     type_ref_name = XTCEManager.BASE_TYPE_NAMESPACE + XTCEManager.NAMESPACE_SEPARATOR + string_type_name
                #     # If the field is a string, then it is a special kind of array that ends in a null terminator.
                #
                #     # member = xtce.MemberType()
                #     # member.set_name(f'{field_name}')
                #     # member.set_typeRef(type_ref_name)
                #     # member_list.add_Member(member)

                # else:
                #     if base_type_val[0] is True:
                #         type_ref_name = type_ref_name
                #     else:
                #         type_ref_name = module_name.rstrip(
                #             XTCEManager.NAMESPACE_SEPARATOR) + XTCEManager.NAMESPACE_SEPARATOR + type_ref_name
                #
                #     new_array = self.__get_array_param_type(field_id, type_ref_name)
                #
                #     if self.__param_basetype_exists(new_array.get_name()) is False:
                #         self[
                #             XTCEManager.BASE_TYPE_NAMESPACE].get_TelemetryMetaData().get_ParameterTypeSet().add_ArrayParameterType(
                #             new_array)

                    # member = xtce.MemberType()
                    # member.set_name(f'{field_name}')
                    # member.set_typeRef(
                    #     XTCEManager.BASE_TYPE_NAMESPACE + XTCEManager.NAMESPACE_SEPARATOR + new_array.get_name())
                    # member_list.add_Member(member)

            else:
                type_ref_name = 'BaseType/UNKNOWN'
                # logging.warning('BaseType/UNKNOWN is being used as array type')

        else:
            # logging.debug('else block')
            # member = xtce.MemberType()
            # member.set_name(field_name)
            symbol_type = db_cursor.execute('SELECT * FROM symbols where id=?',
                                                 (field_type,)).fetchone()
            # The symbol_type is expected, as per our schema, to have the form of (id, elf ,name, byte_size)
            if symbol_type:
                    # logging.debug(f'symbol_type$$$$-->{symbol_type}')
                    base_type_val = __is_base_type(symbol_type[2])

                    if __is__symbol_enum(db_cursor, field_type) is True:
                        # FIXME
                        pass
                        # if self.__enumeration_paramtype_exists(field_type, module_name) is True:
                        #     type_ref_name = self.__get_enum_from_symbol_id(field_type)
                        #
                        # else:


                        # enum_size = __get_enum_byte_size()
                        # out_data[symbol_record][base_type_val[1]] = bytearray()
                        out_data[symbol_name][field_name] = bytearray()
                        for d in data[current_data_cursor:current_data_cursor+symbol_type[3]]:
                            new_current_data_cursor += 1
                            out_data[symbol_name][field_name].append(d)

                    elif base_type_val[0]:
                        #     TODO: Make a distinction between unsigned and int types
                        out_data[symbol_name][field_name] = bytearray()
                        for d in data[current_data_cursor:current_data_cursor + symbol_type[3]]:
                            new_current_data_cursor += 1
                            out_data[symbol_name][field_name].append(d)

                        # out_data[symbol_record][base_type_val[1]] = data[current_data_cursor]
                        # type_ref_name = __get_basetype_name(base_type_val[1], symbol_type[3] * 8,
                        #                                          self.is_little_endian(symbol_type[1]))
                    else:
                        # logging.debug(f'field type-->{field_type}')
                        child_symbol = db_cursor.execute('SELECT * FROM symbols where id=?',
                                                              (field_type,)).fetchone()

                        # logging.debug(f'field_symbol id:{field_symbol}')
                        # logging.debug(f'child symbol-->{child_symbol}')
                        # logging.debug(f'field id-->{field_id})')
                        # child = get_overlayed_type(child_symbol,db_cursor, new_current_data_cursor, data, out_data, new_current_data_cursor)
                        get_overlayed_type(child_symbol, db_cursor, data, new_current_data_cursor, out_data)
                        # if self.__aggregate_paramtype_exists(child_symbol[2], module_name) is False:
                        #     self[module_name].get_TelemetryMetaData().get_ParameterTypeSet().add_AggregateParameterType(
                        #         child)
                        # type_ref_name = child.get_name()
            else:
                type_ref_name = 'BaseType/UNKNOWN'

            # member.set_typeRef(type_ref_name)
            # member_list.add_Member(member)

    # logging.debug(f'out_param--> {out_param.get_name()}')
    # return out_param
    return


def map_data_from_symbol(symbol_value_map: dict, data):
    pass


def get_data_from_symbol_variable(db_cursor: Cursor, symbol_variable_name: str, data: bytes):
    # FIXME:Something we could possibly do is have a foreign key from elf_symbols_table to variables....
    symbol_variable_type = db_cursor.execute("select type from variables where name=?",
                                             (symbol_variable_name,)).fetchone()
    if symbol_variable_type and len(symbol_variable_type) > 0:
        symbol_variable_type_record = db_cursor.execute("select * from symbols where id=?",
                                                        (symbol_variable_type[0],)).fetchone()

        if len(symbol_variable_type_record) > 0:
            out_data = {}

            get_overlayed_type(symbol_variable_type_record, db_cursor, data, 0, out_data)
            print(f"out_data:{out_data}")

    # symbol_variable_name
    pass


def main():
    db_handle = sqlite3.connect(
        "/home/lgomez/projects/documentation/squeaky-weasel/software/airliner/core/tools/auto-yamcs/juicer/build/whl.db")
    db_cursor = db_handle.cursor()
    all_elf_symbols = db_cursor.execute('select * from elf_symbol_table').fetchall()

    for record in all_elf_symbols:
        name_index = record[1]  # index within string table
        elf = record[2]
        value = record[3]  # This is the offset within the section
        size = record[4]
        info = record[5]
        other = record[6]
        section_index = record[7]
        file_offset = record[8]
        str_table_file_offset = record[9]

        if size > 0:
            elf_file_path = None
            elf_file_path_record = db_cursor.execute('select name from elfs where id=?', (elf,)).fetchone()

            if len(elf_file_path_record) > 0:
                elf_file_path = elf_file_path_record[0]
                with open(elf_file_path, 'r+b') as file:
                    file.seek(file_offset + value)
                    # if size == 4:
                    symbol_data = file.read(size)
                        # print(f"value:{symbol_data}")
                        # print(f"value int:{int.from_bytes(symbol_data, 'little')}")
                    file.seek(0)
                    file.seek(str_table_file_offset + name_index)

                    str_data = bytearray()
                    # if size == 4:
                    for b in file.read():
                        b: int
                        if b == 0x00:
                            # FIXME:Look like the decode python function adds its own null-byte, which is why we're ignoring it.
                            # str_data += b.to_bytes(1, "little")
                            break
                        str_data += b.to_bytes(1, "little")

                    # print(str(str_data.decode("UTF-8")))
                    variable_name = str(str_data.decode("UTF-8"))
                    print(variable_name)


                    if symbol_data:
                        get_data_from_symbol_variable(db_cursor, variable_name, symbol_data)

                # print(f"elf_file_path:{elf_file_path}")

            # print(f"record:{record}")


main()
