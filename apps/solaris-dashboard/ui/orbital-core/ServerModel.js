// ==============================================================================
// 🖥️ SERVER MODEL — Backward Compatibility Export & Bridge to ComputationalTerrain
// ==============================================================================
import { ComputationalTerrain } from './ComputationalTerrain.js';

export class ServerModel extends ComputationalTerrain {
  constructor(orbitalScene) {
    super(orbitalScene);
  }
}

export { ComputationalTerrain };
