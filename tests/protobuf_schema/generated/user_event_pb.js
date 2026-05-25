/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-mixed-operators, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars, default-case, jsdoc/require-param*/
"use strict";

var $protobuf = require("protobufjs/minimal");

// Common aliases
var $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
var $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

$root.pulsar = (function() {

    /**
     * Namespace pulsar.
     * @exports pulsar
     * @namespace
     */
    var pulsar = {};

    pulsar.example = (function() {

        /**
         * Namespace example.
         * @memberof pulsar
         * @namespace
         */
        var example = {};

        example.UserEvent = (function() {

            /**
             * Properties of a UserEvent.
             * @typedef {Object} pulsar.example.UserEvent.$Properties
             * @property {string|null} [id] UserEvent id
             * @property {string|null} [name] UserEvent name
             * @property {number|null} [age] UserEvent age
             * @property {Array.<string>|null} [tags] UserEvent tags
             * @property {Array.<Uint8Array>} [$unknowns] Unknown fields preserved while decoding
             */

            /**
             * Properties of a UserEvent.
             * @memberof pulsar.example
             * @interface IUserEvent
             * @augments pulsar.example.UserEvent.$Properties
             * @deprecated Use pulsar.example.UserEvent.$Properties instead.
             */

            /**
             * Shape of a UserEvent.
             * @typedef {pulsar.example.UserEvent.$Properties} pulsar.example.UserEvent.$Shape
             */

            /**
             * Constructs a new UserEvent.
             * @memberof pulsar.example
             * @classdesc Represents a UserEvent.
             * @constructor
             * @param {pulsar.example.UserEvent.$Properties=} [properties] Properties to set
             * @property {Array.<Uint8Array>} [$unknowns] Unknown fields preserved while decoding
             */
            function UserEvent(properties) {
                this.tags = [];
                if (properties)
                    for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                        if (properties[keys[i]] != null && keys[i] !== "__proto__")
                            this[keys[i]] = properties[keys[i]];
            }

            /**
             * UserEvent id.
             * @member {string} id
             * @memberof pulsar.example.UserEvent
             * @instance
             */
            UserEvent.prototype.id = "";

            /**
             * UserEvent name.
             * @member {string} name
             * @memberof pulsar.example.UserEvent
             * @instance
             */
            UserEvent.prototype.name = "";

            /**
             * UserEvent age.
             * @member {number} age
             * @memberof pulsar.example.UserEvent
             * @instance
             */
            UserEvent.prototype.age = 0;

            /**
             * UserEvent tags.
             * @member {Array.<string>} tags
             * @memberof pulsar.example.UserEvent
             * @instance
             */
            UserEvent.prototype.tags = $util.emptyArray;

            /**
             * Creates a new UserEvent instance using the specified properties.
             * @function create
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {pulsar.example.UserEvent.$Properties=} [properties] Properties to set
             * @returns {pulsar.example.UserEvent} UserEvent instance
             * @type {{
             *   (properties: pulsar.example.UserEvent.$Shape): pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape;
             *   (properties?: pulsar.example.UserEvent.$Properties): pulsar.example.UserEvent;
             * }}
             */
            UserEvent.create = function create(properties) {
                return new UserEvent(properties);
            };

            /**
             * Encodes the specified UserEvent message. Does not implicitly {@link pulsar.example.UserEvent.verify|verify} messages.
             * @function encode
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {pulsar.example.UserEvent.$Properties} message UserEvent message or plain object to encode
             * @param {$protobuf.Writer} [writer] Writer to encode to
             * @returns {$protobuf.Writer} Writer
             */
            UserEvent.encode = function encode(message, writer, _depth) {
                if (!writer)
                    writer = $Writer.create();
                if (_depth === undefined)
                    _depth = 0;
                if (_depth > $util.recursionLimit)
                    throw Error("max depth exceeded");
                if (message.id != null && Object.hasOwnProperty.call(message, "id"))
                    writer.uint32(/* id 1, wireType 2 =*/10).string(message.id);
                if (message.name != null && Object.hasOwnProperty.call(message, "name"))
                    writer.uint32(/* id 2, wireType 2 =*/18).string(message.name);
                if (message.age != null && Object.hasOwnProperty.call(message, "age"))
                    writer.uint32(/* id 3, wireType 0 =*/24).int32(message.age);
                if (message.tags != null && message.tags.length)
                    for (var i = 0; i < message.tags.length; ++i)
                        writer.uint32(/* id 4, wireType 2 =*/34).string(message.tags[i]);
                if (message.$unknowns != null && Object.hasOwnProperty.call(message, "$unknowns"))
                    for (var i = 0; i < message.$unknowns.length; ++i)
                        writer.raw(message.$unknowns[i]);
                return writer;
            };

            /**
             * Encodes the specified UserEvent message, length delimited. Does not implicitly {@link pulsar.example.UserEvent.verify|verify} messages.
             * @function encodeDelimited
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {pulsar.example.UserEvent.$Properties} message UserEvent message or plain object to encode
             * @param {$protobuf.Writer} [writer] Writer to encode to
             * @returns {$protobuf.Writer} Writer
             */
            UserEvent.encodeDelimited = function encodeDelimited(message, writer) {
                return this.encode(message, writer && writer.len ? writer.fork() : writer).ldelim();
            };

            /**
             * Decodes a UserEvent message from the specified reader or buffer.
             * @function decode
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
             * @param {number} [length] Message length if known beforehand
             * @returns {pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape} UserEvent
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            UserEvent.decode = function decode(reader, length, _end, _depth, _target) {
                if (!(reader instanceof $Reader))
                    reader = $Reader.create(reader);
                if (_depth === undefined)
                    _depth = 0;
                if (_depth > $Reader.recursionLimit)
                    throw Error("max depth exceeded");
                var end = length === undefined ? reader.len : reader.pos + length, message = _target || new $root.pulsar.example.UserEvent(), value;
                while (reader.pos < end) {
                    var start = reader.pos;
                    var tag = reader.tag();
                    if (tag === _end) {
                        _end = undefined;
                        break;
                    }
                    var wireType = tag & 7;
                    switch (tag >>>= 3) {
                    case 1: {
                            if (wireType !== 2)
                                break;
                            if ((value = reader.string()).length)
                                message.id = value;
                            else
                                delete message.id;
                            continue;
                        }
                    case 2: {
                            if (wireType !== 2)
                                break;
                            if ((value = reader.string()).length)
                                message.name = value;
                            else
                                delete message.name;
                            continue;
                        }
                    case 3: {
                            if (wireType !== 0)
                                break;
                            if (value = reader.int32())
                                message.age = value;
                            else
                                delete message.age;
                            continue;
                        }
                    case 4: {
                            if (wireType !== 2)
                                break;
                            if (!(message.tags && message.tags.length))
                                message.tags = [];
                            message.tags.push(reader.string());
                            continue;
                        }
                    }
                    reader.skipType(wireType, _depth, tag);
                    $util.makeProp(message, "$unknowns", false);
                    (message.$unknowns || (message.$unknowns = [])).push(reader.raw(start, reader.pos));
                }
                if (_end !== undefined)
                    throw Error("missing end group");
                return message;
            };

            /**
             * Decodes a UserEvent message from the specified reader or buffer, length delimited.
             * @function decodeDelimited
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
             * @returns {pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape} UserEvent
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            UserEvent.decodeDelimited = function decodeDelimited(reader) {
                if (!(reader instanceof $Reader))
                    reader = new $Reader(reader);
                return this.decode(reader, reader.uint32());
            };

            /**
             * Verifies a UserEvent message.
             * @function verify
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {Object.<string,*>} message Plain object to verify
             * @returns {string|null} `null` if valid, otherwise the reason why it is not
             */
            UserEvent.verify = function verify(message, _depth) {
                if (typeof message !== "object" || message === null)
                    return "object expected";
                if (_depth === undefined)
                    _depth = 0;
                if (_depth > $util.recursionLimit)
                    return "max depth exceeded";
                if (message.id != null && message.hasOwnProperty("id"))
                    if (!$util.isString(message.id))
                        return "id: string expected";
                if (message.name != null && message.hasOwnProperty("name"))
                    if (!$util.isString(message.name))
                        return "name: string expected";
                if (message.age != null && message.hasOwnProperty("age"))
                    if (!$util.isInteger(message.age))
                        return "age: integer expected";
                if (message.tags != null && message.hasOwnProperty("tags")) {
                    if (!Array.isArray(message.tags))
                        return "tags: array expected";
                    for (var i = 0; i < message.tags.length; ++i)
                        if (!$util.isString(message.tags[i]))
                            return "tags: string[] expected";
                }
                return null;
            };

            /**
             * Creates a UserEvent message from a plain object. Also converts values to their respective internal types.
             * @function fromObject
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {Object.<string,*>} object Plain object
             * @returns {pulsar.example.UserEvent} UserEvent
             */
            UserEvent.fromObject = function fromObject(object, _depth) {
                if (object instanceof $root.pulsar.example.UserEvent)
                    return object;
                if (_depth === undefined)
                    _depth = 0;
                if (_depth > $util.recursionLimit)
                    throw Error("max depth exceeded");
                var message = new $root.pulsar.example.UserEvent();
                if (object.id != null)
                    if (typeof object.id !== "string" || object.id.length)
                        message.id = String(object.id);
                if (object.name != null)
                    if (typeof object.name !== "string" || object.name.length)
                        message.name = String(object.name);
                if (object.age != null)
                    if (Number(object.age) !== 0)
                        message.age = object.age | 0;
                if (object.tags) {
                    if (!Array.isArray(object.tags))
                        throw TypeError(".pulsar.example.UserEvent.tags: array expected");
                    message.tags = Array(object.tags.length);
                    for (var i = 0; i < object.tags.length; ++i)
                        message.tags[i] = String(object.tags[i]);
                }
                return message;
            };

            /**
             * Creates a plain object from a UserEvent message. Also converts values to other types if specified.
             * @function toObject
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {pulsar.example.UserEvent} message UserEvent
             * @param {$protobuf.IConversionOptions} [options] Conversion options
             * @returns {Object.<string,*>} Plain object
             */
            UserEvent.toObject = function toObject(message, options, _depth) {
                if (!options)
                    options = {};
                if (_depth === undefined)
                    _depth = 0;
                if (_depth > $util.recursionLimit)
                    throw Error("max depth exceeded");
                var object = {};
                if (options.arrays || options.defaults)
                    object.tags = [];
                if (options.defaults) {
                    object.id = "";
                    object.name = "";
                    object.age = 0;
                }
                if (message.id != null && message.hasOwnProperty("id"))
                    object.id = message.id;
                if (message.name != null && message.hasOwnProperty("name"))
                    object.name = message.name;
                if (message.age != null && message.hasOwnProperty("age"))
                    object.age = message.age;
                if (message.tags && message.tags.length) {
                    object.tags = Array(message.tags.length);
                    for (var j = 0; j < message.tags.length; ++j)
                        object.tags[j] = message.tags[j];
                }
                return object;
            };

            /**
             * Converts this UserEvent to JSON.
             * @function toJSON
             * @memberof pulsar.example.UserEvent
             * @instance
             * @returns {Object.<string,*>} JSON object
             */
            UserEvent.prototype.toJSON = function toJSON() {
                return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
            };

            /**
             * Gets the type url for UserEvent
             * @function getTypeUrl
             * @memberof pulsar.example.UserEvent
             * @static
             * @param {string} [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns {string} The type url
             */
            UserEvent.getTypeUrl = function getTypeUrl(prefix) {
                if (prefix === undefined)
                    prefix = "type.googleapis.com";
                return prefix + "/pulsar.example.UserEvent";
            };

            return UserEvent;
        })();

        return example;
    })();

    return pulsar;
})();

module.exports = $root;
