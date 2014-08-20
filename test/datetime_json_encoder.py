#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
from datetime import *


class DatetimeEncoder(json.JSONEncoder):
    def default( self, obj ):
        if  isinstance( obj, datetime ):
            return obj.strftime("%Y-%m-%d %H:%M:%S")
        if  isinstance( obj, date ):
            return obj.strftime("%Y-%m-%d")
        if  isinstance( obj, time ):
            return obj.strftime("%H:%M:%S")

        return json.JSONEncoder.default( self, obj )
