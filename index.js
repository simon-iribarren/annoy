"use strict";

const binding = require("./binding");

class AnnoyIndex {
  constructor(dimension, metric) {
    if (!["euclidean", "angular"].includes(metric)) {
      throw new Error("Invalid metric, choose 'euclidean' or 'angular'.");
    }

    this._index = binding.annoyCreateIndex(dimension, metric);
  }

  addItem(index, vector) {
    binding.annoyAddItem(this._index, index, vector);
  }

  build(numTrees = -1) {
    binding.annoyBuildIndex(this._index, numTrees);
  }

  getNnsByItem(item, n) {
    return binding.annoyGetNnsByItem(this._index, item, n);
  }

  getNnsByVector(vector, n) {
    return binding.annoyGetNnsByVector(this._index, vector, n);
  }
}

module.exports = AnnoyIndex;