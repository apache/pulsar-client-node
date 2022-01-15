const PulsarBinding = require('bindings')('Pulsar');

class AuthenticationOauth2 {
  constructor(params) {
    this.binding = new PulsarBinding.Authentication('oauth2', params);
  }
}

module.exports = AuthenticationOauth2;
