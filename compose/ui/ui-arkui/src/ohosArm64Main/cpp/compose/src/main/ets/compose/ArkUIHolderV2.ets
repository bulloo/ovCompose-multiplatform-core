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

import { ArkUIViewV2 } from './ArkUIViewV2'

export type HolderV2 = ArkUIViewV2

@Builder
export function holderBuilderV2(holder: HolderV2) {
  ArkUIHolder({ holder: holder, builder: holder.builder, state: holder.parameter })
}

const EmptyObject: object = new Object()

@Component
export struct ArkUIHolder {
  @Require holder!: ArkUIViewV2
  @Require @Provide("compose_args") state: object = EmptyObject
  @BuilderParam builder: (param: object) => void

  build() {
    Stack() {
      this.builder(this.state)
    }
  }

  onMeasureSize(selfLayoutInfo: GeometryInfo, children: Measurable[], constraint: ConstraintSizeOptions): SizeResult {
      return children[0].measure(constraint)
  }
}