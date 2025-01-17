// TODO(jpienaar): Re-enable
// DISABLED: tfg-transforms-opt -pass-pipeline='tfg-lift-graph-to-func{feeds=Placeholder1:0,Placeholder2:0 fetches=SomeAdd3:0 control_rets=SomeAdd4}' %s | FileCheck %s

// Test that we can lift the graph into a function by using the provided feeds
// as function arguments and the provided fetch as function results.

// CHECK:   tfg.func @_mlir_lifted_graph(%Placeholder13A0: tensor<*xf32> {tfg.name = "Placeholder1:0"},
// CHECK-NEXT:                           %Placeholder23A0: tensor<*xf32> {tfg.name = "Placeholder2:0"})

// Feeds substitution:
// CHECK: tfg.Add(%Placeholder13A0, %Placeholder23A0) name("SomeAdd1")

// Fetch:
// CHECK: %[[FETCH:.*]], %ctl_6 = tfg.Add{{.*}} name("SomeAdd3")
// CHECK: %ctl_[[CTL:.*]] = tfg.Add{{.*}} name("SomeAdd4")
// CHECK: tfg.return(%[[FETCH]]) [%ctl_[[CTL]]]

tfg.graph #tf_type.version<producer = 34, min_consumer = 5> {
  %Placeholder, %ctl_0 = tfg.Placeholder name("Placeholder1") {dtype = i32} : () -> (tensor<*xf32>)
  %Placeholder_1, %ctl_1 = tfg.Placeholder name("Placeholder2") {dtype = i32} : () -> (tensor<*xf32>)
  %add1, %ctl2 = tfg.Add(%Placeholder, %Placeholder_1) name("SomeAdd1") {T = i32} : (tensor<*xf32>, tensor<*xf32>) -> (tensor<*xf32>)
  %add2, %ctl3 = tfg.Add(%add1, %Placeholder_1) name("SomeAdd2") {T = i32} : (tensor<*xf32>, tensor<*xf32>) -> (tensor<*xf32>)
  %add3, %ctl4 = tfg.Add(%Placeholder, %add2) name("SomeAdd3") {T = i32} : (tensor<*xf32>, tensor<*xf32>) -> (tensor<*xf32>)
  %add4, %ctl5 = tfg.Add(%add3, %add1) name("SomeAdd4") {T = i32} : (tensor<*xf32>, tensor<*xf32>) -> (tensor<*xf32>)
  %add5, %ctl6 = tfg.Add(%Placeholder, %add4) name("SomeAdd5") {T = i32} : (tensor<*xf32>, tensor<*xf32>) -> (tensor<*xf32>)
}
