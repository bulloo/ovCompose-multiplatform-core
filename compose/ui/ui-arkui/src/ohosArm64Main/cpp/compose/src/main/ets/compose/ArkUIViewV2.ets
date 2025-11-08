/*
 * Tencent is pleased to support the open source community by making ovCompose available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { BuilderNode, UIContext } from '@kit.ArkUI'
import { holderBuilderV2, HolderV2 } from './ArkUIHolderV2'

const interopBuilderTable = new Map<string, Function>()

export function registerComposeInteropBuilder(key: string, builder: Function) {
  interopBuilderTable.set(key, builder)
}

let globalComposeNodeId = 0

type ArkUIBuilder = (args: object) => void

export class ArkUIViewV2 {
  id: number = globalComposeNodeId++
  name: string
  parameter: Object
  builder: ArkUIBuilder
  frameNode: FrameNode | null = null

  constructor(context: UIContext, name: string, parameter: Object) {
    this.name = name
    this.parameter = parameter
    const composable = interopBuilderTable.get(name)
    if (composable) {
      this.builder = composable as ArkUIBuilder
    } else {
      throw new Error(`ArkUI Composable interop builder named: ${name} is not found. Please register it before initializing Compose.`)
    }

    // WrappedBuilder & wrapBuilder are accessible in ets.
    const builder = wrapBuilder<[HolderV2]>(holderBuilderV2)
    const builderNode = new BuilderNode<[HolderV2]>(context)
    builderNode.build(builder, this)
    this.frameNode = builderNode.getFrameNode()
  }
}
