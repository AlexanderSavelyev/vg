document.addEventListener('DOMContentLoaded', function(){ // on dom ready
  console.log("start");
  // var create_cy = 
  $.getJSON( "data.json")
    .success( function(data) {
        var cy = cytoscape({
        container: document.querySelector('#cy'),
          
        boxSelectionEnabled: false,
        autounselectify: true,
        
        style: cytoscape.stylesheet()
          .selector('node')
            .css({
              'content': 'data(name)',
              'text-valign': 'center',
              'color': 'white',
              'text-outline-width': 2,
              'background-color': 'data(color)',
              'text-outline-color': '#999'
            })
          .selector('edge')
            .css({
              'curve-style': 'bezier',
              'target-arrow-shape': 'triangle',
              'target-arrow-color': '#ccc',
              'line-color': '#ccc',
              'label': 'data(label)',
              'width': 1
            })
          .selector(':selected')
            .css({
              'background-color': 'black',
              'line-color': 'black',
              'target-arrow-color': 'black',
              'source-arrow-color': 'black'
            })
          .selector('.faded')
            .css({
              'opacity': 0.25,
              'text-opacity': 0
            }),
        
        elements: data.graph,
        
        layout: {
          name: 'breadthfirst',
          padding: 10,
          roots: data.reference
        }
      });

      cy.on('tap', 'node', function(e){
        var node = e.cyTarget; 
        var neighborhood = node.neighborhood().add(node);
        
        cy.elements().addClass('faded');
        neighborhood.removeClass('faded');
      });

      cy.on('tap', function(e){
        if( e.cyTarget === cy ){
          cy.elements().removeClass('faded');
        }
      });
    })
    .error(function(err) {
      console.log("Error"); 
      console.log(err);
    });
    



}); // on dom ready