var lyngkModule = (function() {
  my = {};
  var verticalX = [1, 2, 3, 4, 5, 6, 7];
  var verticalY1 = [3, 0, 1, 0, 1, 0, 3];
  var verticalY2 = [9, 12, 11, 12, 11, 12, 9];
  var diagonalSourceCoords = [[1, 3], [0, 6], [1, 7], [1, 9], [2, 10], [2, 12], [4, 12], [4, 0], [2, 0], [2, 2], [1, 3], [1, 5], [0, 6], [1, 9]];
  var diagonalDestination = [[4, 0], [6, 0], [6, 2], [7, 3], [7, 5], [8, 6], [7, 9], [7, 3], [8, 6], [7, 7], [7, 9], [6, 10], [6, 12], [4, 12]];
  var xRoot = Math.sqrt(3) * 25;
  var yOffset = 25;
  var padding = 25;
  var boardPosition = {
    A1: [0, 6],
    B1: [1, 9],
    B2: [1, 7],
    B3: [1, 5],
    B4: [1, 3],
    C1: [2, 12],
    C2: [2, 10],
    C3: [2, 8],
    C4: [2, 6],
    C5: [2, 4],
    C6: [2, 2],
    C7: [2, 0],
    D1: [3, 11],
    D2: [3, 9],
    D3: [3, 7],
    D4: [3, 5],
    D5: [3, 3],
    D6: [3, 1],
    E1: [4, 12],
    E2: [4, 10],
    E3: [4, 8],
    E4: [4, 6],
    E5: [4, 4],
    E6: [4, 2],
    E7: [4, 0],
    F1: [5, 11],
    F2: [5, 9],
    F3: [5, 7],
    F4: [5, 5],
    F5: [5, 3],
    F6: [5, 1],
    G1: [6, 12],
    G2: [6, 10],
    G3: [6, 8],
    G4: [6, 6],
    G5: [6, 4],
    G6: [6, 2],
    G7: [6, 0],
    H1: [7, 9],
    H2: [7, 7],
    H3: [7, 5],
    H4: [7, 3],
    I1: [8, 6]
  };
  var nodeList = ['A1', 'B1', 'B2', 'B3', 'B4', 'C1', 'C2', 'C3', 'C4', 'C5', 'C6', 'C7', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'E1', 'E2', 'E3', 'E4', 'E5', 'E6', 'E7', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'G1', 'G2', 'G3', 'G4', 'G5', 'G6', 'G7', 'H1', 'H2', 'H3', 'H4', 'I1'];
  var colorValues = {
    Ivory: "#ffffc8",
    Blue: "blue",
    Red: "red",
    Green: "green",
    Black: "black",
    White: "gray"
  };
  var c=document.getElementById("lyngkboard");
  var context=c.getContext("2d");
  var letterToColor = {
    'R': "Red",
    'K': "Black",
    'B': "Blue",
    'I': "Ivory",
    'G': "Green",
    'J': "White",
    '-': "Empty"
  };
  var colorArray = ['R', 'K', 'B', 'I', 'G'];
  var kTestString1 = 'KRI|-|G|-|GR|-|IG|KJ|-|-|-|-|RK|-|J|KB|I|-|-|-|-|K|G|-|I|GBI|-|I|GIJR|B|-|RB|RKBI|G|RK|-|GB|-|-|RB|-|BK|-';

  var kTestString = 'G|B|K|K|R|K|I|I|K|G|J|K|K|B|I|B|G|R|G|K|R|G|K|J|B|R|R|B|G|J|G|I|G|B|R|B|I|I|R|B|I|R|I|';
  my.startUp = drawBoard(context);

  function drawBoard(ctx) {
    ctx.clearRect(0, 0, c.width, c.height);
    var colorCount = {
      Ivory: 8,
      Blue: 8,
      Red: 8,
      Green: 8,
      Black: 8,
      White: 3
    };
    
    for (var i = 0; i < verticalX.length; i++) {
      drawLine((xRoot * verticalX[i] + padding), (yOffset * verticalY1[i] + padding), (xRoot * verticalX[i] + padding), (yOffset * verticalY2[i] + padding), ctx);
    }
    for (var j = 0; j < diagonalDestination.length; j++) {
      drawLine((xRoot * diagonalSourceCoords[j][0] + padding), (yOffset * diagonalSourceCoords[j][1] + padding), (xRoot * diagonalDestination[j][0] + padding), (yOffset * diagonalDestination[j][1] + padding), ctx);
    }
    var boardString = document.getElementById('boardstring').innerHTML;
    translateBoardString(boardString, context);
    populateClaimedCanvases();
  }

  function drawLine(x1, y1, x2, y2, ctx) {
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  }

  function drawGamePiece(xcenter, ycenter, color, ctx) {
    ctx.beginPath();
    ctx.arc(xcenter, ycenter, 10, 0, 2*Math.PI);
    ctx.fillStyle = colorValues[color];
    ctx.fill();
    ctx.stroke();
  }

  function drawUnclaimedPiece(color) {
    var board =document.getElementById("unclaimedCanvas");
    var ctx=board.getContext("2d");
    var piecePosition = 0;
    switch(color) {
      case 'R':
        piecePosition = 1;
        break;
      case 'K':
        piecePosition = 2;
        break;
      case 'B':
        piecePosition = 3;
        break;
      case 'I':
        piecePosition = 4;
        break;
      case 'G':
        piecePosition = 5;
        break;
    }
    ctx.beginPath();
    ctx.arc(40, piecePosition * 50, 10, 0, 2*Math.PI);
    ctx.fillStyle = colorValues[letterToColor[color]];
    ctx.fill();
    ctx.stroke();
  }

  function drawClaimedPiece(color, boardNumber) {
    boardLabel = '';
    switch(boardNumber) {
      case 1:
        boardLabel = "player1canvas";
        break;
      case 2:
        boardLabel = "player2canvas";
        break;
    }
    var board =document.getElementById(boardLabel);
    var ctx=board.getContext("2d");
    var piecePosition = 0;
    switch(color) {
      case 'R':
        piecePosition = 1;
        break;
      case 'K':
        piecePosition = 2;
        break;
      case 'B':
        piecePosition = 3;
        break;
      case 'I':
        piecePosition = 4;
        break;
      case 'G':
        piecePosition = 5;
        break;
    }
    ctx.beginPath();
    ctx.arc(piecePosition * 29, 25, 10, 0, 2*Math.PI);
    ctx.fillStyle = colorValues[letterToColor[color]];
    ctx.fill();
    ctx.stroke();
  }

  function populateClaimedCanvases() {
    var unclaimedBoard = document.getElementById("unclaimedCanvas");
    var unclaimedctx = unclaimedBoard.getContext("2d");
    unclaimedctx.clearRect(0, 0, unclaimedBoard.width, unclaimedBoard.height);
    var claimedBoard1 = document.getElementById("player1canvas");
    var player1ctx = claimedBoard1.getContext("2d");
    player1ctx.clearRect(0, 0, claimedBoard1.width, claimedBoard1.height);
    var claimedBoard2 = document.getElementById("player2canvas");
    var player2ctx = claimedBoard2.getContext("2d");
    player2ctx.clearRect(0, 0, claimedBoard2.width, claimedBoard2.height);
    var stringArray1 = document.getElementById('player1claimed').innerHTML.split('');
    var stringArray2 = document.getElementById('player2claimed').innerHTML.split('');
    for(var i = 0; i < colorArray.length; i++) {
      if (stringArray1.indexOf(colorArray[i]) != -1) {
        drawClaimedPiece(colorArray[i], 1);
      }
      else if (stringArray2.indexOf(colorArray[i]) != -1) {
        drawClaimedPiece(colorArray[i], 2);
      }
      else {
        drawUnclaimedPiece(colorArray[i]);
      }
    }
  }

  function stackGamePiece(xcenter, ycenter, color, stackCount, ctx) {
    stackOffset = (stackCount - 1) * 5;
    drawGamePiece(xcenter, ycenter - stackOffset, color, ctx);
  }

  function shuffleArray(input) {
    var inputArray = input;
    for(var i = inputArray.length; i > 0; i--) {
      randomIndex = Math.random() * (i - 1);
      x = inputArray[randomIndex];
      inputArray[randomIndex] = inputArray[i];
      inputArray[i] = x;
    }
    return inputArray;
  }

  function translateBoardString(boardString, ctx) {
    var boardArray = boardString.split('|');
    for (var i = 0; i < boardArray.length; i++) {
      for (var j = 0; j < boardArray[i].length; j++) {
        var boardIndex = nodeList[i];
        var color = letterToColor[boardArray[i][j]];
        if (color != "Empty") {
          stackGamePiece((xRoot * boardPosition[boardIndex][0] + padding), (yOffset * boardPosition[boardIndex][1] + padding), color, j + 1, ctx);
        }
      }
    }
  }
  return my;
})();

window.onload = lyngkModule.startUp;
