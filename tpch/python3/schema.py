ENUM_L_RETURNFLAG = ("A", "N", "R")
ENUM_L_LINESTATUS = ("F", "O")
ENUM_O_ORDERSTATUS = ("F", "O", "P")
ENUM_P_TYPE = list("%s %s %s" % (i, j, k)
                   for i in ("STANDARD", "SMALL", "MEDIUM", "LARGE", "ECONOMY",
                             "PROMO")
                   for j in ("ANODIZED", "BURNISHED", "PLATED", "POLISHED",
                             "BRUSHED")
                   for k in ("TIN", "NICKEL", "BRASS", "STEEL", "COPPER"))
ENUM_P_CONTAINER = list("%s %s" % (i, j)
                        for i in ("SM", "LG", "MED", "JUMBO", "WRAP")
                        for j in ("CASE", "BOX", "BAG", "JAR", "PKG", "PACK",
                                  "CAN", "DRUM"))
ENUM_C_MKTSEGMENT = ("AUTOMOBILE", "BUILDING", "FURNITURE", "MACHINERY",
                     "HOUSEHOLD")
ENUM_O_ORDERPRIORITY = ("1-URGENT", "2-HIGH", "3-MEDIUM", "4-NOT SPECIFIED",
                        "5-LOW")
ENUM_P_MFGR = list("Manufacturer#%d" % i for i in range(1, 6))
ENUM_P_BRAND = list("Brand#%d%d" % (i, j) for i in range(1, 6)
                    for j in range(1, 6))
ENUM_L_SHIPINSTRUCT = ("DELIVER IN PERSON", "COLLECT COD", "NONE",
                       "TAKE BACK RETURN")
ENUM_L_SHIPMODE = ("REG AIR", "AIR", "RAIL", "SHIP", "TRUCK", "MAIL", "FOB")

LIST_N = (("ALGERIA", 0), ("ARGENTINA", 1), ("BRAZIL", 1), ("CANADA", 1),
          ("EGYPT", 4), ("ETHIOPIA", 0), ("FRANCE", 3), ("GERMANY", 3),
          ("INDIA", 2), ("INDONESIA", 2), ("IRAN", 4), ("IRAQ", 4),
          ("JAPAN", 2), ("JORDAN", 4), ("KENYA", 0), ("MOROCCO", 0),
          ("MOZAMBIQUE", 0), ("PERU", 1), ("CHINA", 2), ("ROMANIA", 3),
          ("SAUDI ARABIA", 4), ("VIETNAM", 2), ("RUSSIA",
                                                3), ("UNITED KINGDOM",
                                                     3), ("UNITED STATES", 1))
LIST_R_NAME = ("AFRICA", "AMERICA", "ASIA", "EUROPE", "MIDDLE EAST")

SCHEMA = {
    "lineitem": {
        "l_orderkey": ["int32"],
        "l_partkey": ["int32"],
        "l_suppkey": ["int32"],
        "l_linenumber": ["int32"],
        "l_quantity": ["decimal"],
        "l_extendedprice": ["decimal"],
        "l_discount": ["decimal"],
        "l_tax": ["decimal"],
        "l_returnflag": ["int8", ENUM_L_RETURNFLAG],
        "l_linestatus": ["int8", ENUM_L_LINESTATUS],
        "l_shipdate": ["date"],
        "l_commitdate": ["date"],
        "l_receiptdate": ["date"],
        "l_shipinstruct": ["int8", ENUM_L_SHIPINSTRUCT],
        "l_shipmode": ["int8", ENUM_L_SHIPMODE],
        "l_comment": ["string"],
    },
    "nation": {
        "n_nationkey": ["int32"],
        "n_name": ["string"],
        "n_regionkey": ["int32"],
        "n_comment": ["string"],
    },
    "order": {
        "o_orderkey": ["int32"],
        "o_custkey": ["int32"],
        "o_orderstatus": ["int8", ENUM_O_ORDERSTATUS],
        "o_totalprice": ["decimal"],
        "o_orderdate": ["date"],
        "o_orderpriority": ["int8", ENUM_O_ORDERPRIORITY],
        "o_clerk": ["string"],
        "o_shippriority": ["int32"],
        "o_comment": ["string"],
    },
    "region": {
        "r_regionkey": ["int32"],
        "r_name": ["string"],
        "r_comment": ["string"],
    },
    "part": {
        "p_partkey": ["int32"],
        "p_name": ["string"],
        "p_mfgr": ["int8", ENUM_P_MFGR],
        "p_brand": ["int8", ENUM_P_BRAND],
        "p_type": ["int16", ENUM_P_TYPE],
        "p_size": ["int32"],
        "p_container": ["int8", ENUM_P_CONTAINER],
        "p_retailprice": ["decimal"],
        "p_comment": ["string"],
    },
    "customer": {
        "c_custkey": ["int32"],
        "c_name": ["string"],
        "c_address": ["string"],
        "c_nationkey": ["int32"],
        "c_phone": ["string"],
        "c_acctbal": ["decimal"],
        "c_mktsegment": ["int8", ENUM_C_MKTSEGMENT],
        "c_comment": ["string"],
    },
    "supplier": {
        "s_suppkey": ["int32"],
        "s_name": ["string"],
        "s_address": ["string"],
        "s_nationkey": ["int32"],
        "s_phone": ["string"],
        "s_acctbal": ["decimal"],
        "s_comment": ["string"],
    },
    "partsupp": {
        "ps_partkey": ["int32"],
        "ps_suppkey": ["int32"],
        "ps_availqty": ["int32"],
        "ps_supplycost": ["decimal"],
        "ps_comment": ["string"],
    },
}
