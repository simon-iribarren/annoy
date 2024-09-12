"use strict";

const AnnoyIndex = require("./index");

function main() {
  console.log("Running example");
  console.log(AnnoyIndex);
  // Create an AnnoyIndex for 3-dimensional vectors using the 'angular' metric
  const a = new AnnoyIndex(3, "angular");
  console.log(a)

  // Add items (vectors) to the index
  a.addItem(0, [1, 0, 0]);
  a.addItem(1, [0, 1, 0]);
  a.addItem(2, [0, 0, 1]);

  // Build the index (using default -1 trees which means auto-selection of trees)
  a.build();

  // Get the nearest neighbors for the item with index 0
  console.log("Nearest neighbors to item 0:", a.getNnsByItem(0, 100));

  // Get the nearest neighbors for a specific vector
  console.log(
    "Nearest neighbors to vector [1.0, 0.5, 0.5]:",
    a.getNnsByVector([1.0, 0.5, 0.5], 100)
  );
}


main()