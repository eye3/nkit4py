#!/usr/bin/env python
# -*- coding: utf-8 -*-

spec_str = """[
    "offer", {
        "id->_": "string",
        "location->_": "string",
        "note->_": "string"
    }
]"""

xml_str = """<?xml version="1.0"?>
<commerce>
    <offer>
        <id>150016102</id>
        <realty_type>F</realty_type>
        <deal_type>S</deal_type>
        <location type="П">Адмиралтейский</location>
        <address>3 Красноармейская ул., д.13</address>
        <agency_id>2225</agency_id>
        <area total="147.0" />
        <currency>RUR</currency>
        <price>12012000</price>
        <photo>http://img.eip.ru/img/obj/e/08/59/18949192.jpg</photo>
        <note>Любое назначение, 2 отдельных входа с улицы. ПП. Возможна сдача помещения в аренду.</note>
    </offer>
    <offer>
        <id>150015407</id>
        <realty_type>F</realty_type>
        <deal_type>S</deal_type>
        <location type="П">Адмиралтейский</location>
        <address>Адмиралтейский район</address>
        <agency_id>2225</agency_id>
        <area total="204.0" />
        <currency>RUR</currency>
        <price>44000000</price>
        <photo>http://img.eip.ru/img/obj/e/98/88/18948171.jpg</photo>
        <note>Продажа коммерческого помещения</note>
    </offer>
</commerce>"""