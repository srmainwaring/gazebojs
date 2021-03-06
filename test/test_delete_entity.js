const assert = require('assert')
const  spawn = require('child_process').spawn
const  gazebojs = require('../index')
const  model_uri = 'model://coke_can'
const timing = require('./timing.js').del

suite('deletion', function() {

    var gzserver;
    var gazebo;

    this.timeout(timing.test);

    suiteSetup (function(done){

        gzserver = spawn('gzserver', ['--verbose']);
        gzserver.on('data', (data) => { console.log('gz: ' + data) })
        // give a second for gzserver to come up
        setTimeout(()=> {
            gazebo = new gazebojs.Gazebo();
            gazebo.proc = gzserver
            console.log('sim pid: ' + gazebo.proc.pid)
            done();
        }, timing.spawn);
    });

    // Test deletion of an entity.
    test('Delete an entity from using gazebo prototype', function(done) {
        gazebo.subscribe('gazebo.msgs.Response', '~/response', function(e,d){
            assert(d.response === 'success' && d.request === 'entity_delete');
            gazebo.unsubscribe('~/response');
            done();
        });
        gazebo.spawn(model_uri, 'coke_can');
        setTimeout(()=>{
            gazebo.deleteEntity('coke_can');
        }, timing.cmd)
    });

    suiteTeardown(function() {
        console.log('suiteTeardown');
        gzserver.kill('SIGHUP');
    });

});
