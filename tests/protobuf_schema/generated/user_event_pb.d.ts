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

import * as $protobuf from "protobufjs";
import Long = require("long");

/** Namespace pulsar. */
export namespace pulsar {

    /** Namespace example. */
    namespace example {

        /**
         * Properties of a UserEvent.
         * @deprecated Use pulsar.example.UserEvent.$Properties instead.
         */
        interface IUserEvent extends pulsar.example.UserEvent.$Properties {
        }

        /** Represents a UserEvent. */
        class UserEvent {

            /**
             * Constructs a new UserEvent.
             * @param [properties] Properties to set
             */
            constructor(properties?: pulsar.example.UserEvent.$Properties);

            /** Unknown fields preserved while decoding */
            $unknowns?: Uint8Array[];

            /** UserEvent id. */
            id: string;

            /** UserEvent name. */
            name: string;

            /** UserEvent age. */
            age: number;

            /** UserEvent tags. */
            tags: string[];

            /**
             * Creates a new UserEvent instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UserEvent instance
             */
            static create(properties: pulsar.example.UserEvent.$Shape): pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape;
            static create(properties?: pulsar.example.UserEvent.$Properties): pulsar.example.UserEvent;

            /**
             * Encodes the specified UserEvent message. Does not implicitly {@link pulsar.example.UserEvent.verify|verify} messages.
             * @param message UserEvent message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: pulsar.example.UserEvent.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UserEvent message, length delimited. Does not implicitly {@link pulsar.example.UserEvent.verify|verify} messages.
             * @param message UserEvent message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: pulsar.example.UserEvent.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a UserEvent message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape} UserEvent
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape;

            /**
             * Decodes a UserEvent message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape} UserEvent
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): pulsar.example.UserEvent & pulsar.example.UserEvent.$Shape;

            /**
             * Verifies a UserEvent message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a UserEvent message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UserEvent
             */
            static fromObject(object: { [k: string]: any }): pulsar.example.UserEvent;

            /**
             * Creates a plain object from a UserEvent message. Also converts values to other types if specified.
             * @param message UserEvent
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: pulsar.example.UserEvent, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UserEvent to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UserEvent
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UserEvent {

            /** Properties of a UserEvent. */
            interface $Properties {

                /** UserEvent id */
                id?: (string|null);

                /** UserEvent name */
                name?: (string|null);

                /** UserEvent age */
                age?: (number|null);

                /** UserEvent tags */
                tags?: (string[]|null);

                /** Unknown fields preserved while decoding */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a UserEvent. */
            type $Shape = pulsar.example.UserEvent.$Properties;
        }
    }
}
