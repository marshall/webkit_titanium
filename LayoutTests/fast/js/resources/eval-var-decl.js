description(
"This test case checks whether variables cause properties to be defined even before reaching the declaration statement in various cases."
);

shouldBeTrue('hasOwnProperty("foo")');
var foo = 3;

delete bar;
shouldBeTrue('hasOwnProperty("bar")');
var bar = 3;

var firstEvalResult = eval('var result = hasOwnProperty("y"); var y = 3; result');
shouldBeTrue("firstEvalResult");

var secondEvalResult = eval('delete x; var result = hasOwnProperty("x"); var x = 3; result');
shouldBeFalse("secondEvalResult");

var successfullyParsed = true;
